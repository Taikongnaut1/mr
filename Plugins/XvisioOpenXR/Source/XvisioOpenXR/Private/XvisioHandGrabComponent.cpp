#include "XvisioHandGrabComponent.h"

#include "Standalone/XvisioOpenXR.h"
#include "XvisioGrabbableInterface.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"

UXvisioHandGrabComponent::UXvisioHandGrabComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UXvisioHandGrabComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UXvisioHandGrabComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 1. 获取手部关节数据
	TArray<FVector> Positions;
	TArray<FQuat> Rotations;
	TArray<float> Radii;

	if (!UXvisioOpenXR::GetHandJointExtraData(HandType, Positions, Rotations, Radii))
	{
		return;
	}

	if (Positions.Num() <= IndexTipJointIndex || Positions.Num() <= ThumbTipJointIndex)
	{
		return;
	}

	// 计算捏合点（拇指尖与食指尖中点）
	const FVector PinchPoint = GetPinchPoint(Positions);

	// HMD 距离检查：手太靠近眼镜时数据不稳定，跳过
	if (IsHandTooCloseToHMD(PinchPoint))
	{
		// 如果正在抓取，先释放
		if (GrabbedActor.IsValid())
		{
			ReleaseObject();
		}
		bIsPinching = false;
		bWasPinching = false;
		return;
	}

	// 2. 捏合检测
	const bool bNowPinching = CheckPinchGesture(Positions);

	// 3. 状态机
	if (bNowPinching && !bWasPinching)
	{
		// 捏合开始
		bIsPinching = true;
		bWasPinching = true;
		LastPinchLocation = PinchPoint;

		// 在捏合点周围查找可抓取物体
		AActor* Nearest = FindNearestGrabbable(PinchPoint);
		if (Nearest)
		{
			GrabObject(Nearest, PinchPoint);
		}

		OnPinchStart.Broadcast(HandType);
	}
	else if (bNowPinching && bWasPinching)
	{
		// 捏合持续中
		if (GrabbedActor.IsValid())
		{
			// 用食指方向旋转来带动物体旋转
			FQuat HandRotation = Rotations[IndexTipJointIndex];
			UpdateGrabbedObject(PinchPoint, HandRotation);
		}
		else
		{
			// 没抓到东西时持续检测
			AActor* Nearest = FindNearestGrabbable(PinchPoint);
			if (Nearest)
			{
				GrabObject(Nearest, PinchPoint);
			}
		}

		LastPinchLocation = PinchPoint;
	}
	else if (!bNowPinching && bWasPinching)
	{
		// 捏合释放
		bIsPinching = false;
		bWasPinching = false;

		if (GrabbedActor.IsValid())
		{
			ReleaseObject();
		}

		OnPinchEnd.Broadcast(HandType);
	}

	// Debug 绘制
	if (bDrawDebug)
	{
		UKismetSystemLibrary::DrawDebugSphere(
			GetWorld(), PinchPoint,
			GrabRadius, 12,
			bNowPinching ? FLinearColor::Green : FLinearColor::Yellow,
			0.0f, 1.0f);

		// DrawDebugString removed: UE5.4 signature mismatch in this toolchain.
		// The sphere indicator above is sufficient for debug visualization.
	}
}

bool UXvisioHandGrabComponent::CheckPinchGesture(const TArray<FVector>& Positions)
{
	const FVector& ThumbPos = Positions[ThumbTipJointIndex];
	const FVector& IndexPos = Positions[IndexTipJointIndex];
	const float Distance = FVector::Dist(ThumbPos, IndexPos);
	return Distance < PinchThreshold;
}

FVector UXvisioHandGrabComponent::GetPinchPoint(const TArray<FVector>& Positions) const
{
	return (Positions[ThumbTipJointIndex] + Positions[IndexTipJointIndex]) * 0.5f;
}

bool UXvisioHandGrabComponent::IsHandTooCloseToHMD(const FVector& HandPosition) const
{
	// UE5.4: GetOrientationAndPosition returns void, not bool.
	// Guard with IsHeadMountedDisplayEnabled to avoid invalid HMD data.
	if (!UHeadMountedDisplayFunctionLibrary::IsHeadMountedDisplayEnabled())
	{
		return false;
	}

	FVector HMDPosition;
	FRotator HMDRotation;
	UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(HMDRotation, HMDPosition);
	const float Dist = FVector::Dist(HandPosition, HMDPosition);
	return Dist < MinHMDDistance;
}

AActor* UXvisioHandGrabComponent::FindNearestGrabbable(const FVector& PinchPosition)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// 扩大碰撞通道：包含 WorldStatic、WorldDynamic、PhysicsBody
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_PhysicsBody));

	TArray<AActor*> ActorsToIgnore;
	if (GetOwner())
	{
		ActorsToIgnore.Add(GetOwner());
	}
	// 如果另一只手正抓着物体，也排除
	if (GrabbedActor.IsValid())
	{
		ActorsToIgnore.Add(GrabbedActor.Get());
	}

	TArray<AActor*> OverlappingActors;
	UKismetSystemLibrary::SphereOverlapActors(
		World,
		PinchPosition,
		GrabRadius,
		ObjectTypes,
		AActor::StaticClass(),
		ActorsToIgnore,
		OverlappingActors);

	// 筛选可抓取对象
	TArray<AActor*> Candidates;
	for (AActor* Actor : OverlappingActors)
	{
		if (Actor && IsGrabbable(Actor))
		{
			Candidates.Add(Actor);
		}
	}

	if (Candidates.Num() == 0)
	{
		return nullptr;
	}

	// 按距离排序，取最近
	Candidates.Sort([&PinchPosition](const AActor& A, const AActor& B)
	{
		return FVector::DistSquared(A.GetActorLocation(), PinchPosition) <
			   FVector::DistSquared(B.GetActorLocation(), PinchPosition);
	});

	return Candidates[0];
}

void UXvisioHandGrabComponent::GrabObject(AActor* Actor, const FVector& PinchPosition)
{
	if (!Actor)
	{
		return;
	}

	GrabbedActor = Actor;

	// 记录物体与捏合点的偏移（这样抓取时物体不会瞬移到手上）
	GrabOffsetLocation = Actor->GetActorLocation() - PinchPosition;
	GrabOffsetRotation = Actor->GetActorQuat();

	// 关闭物理模拟以便直接控制位置
	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (RootPrimitive)
	{
		RootPrimitive->SetSimulatePhysics(false);
		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	LastGrabbedLocation = Actor->GetActorLocation();
	PrevGrabbedLocation = LastGrabbedLocation;

	// 通知接口
	if (Actor->Implements<UXvisioGrabbableInterface>())
	{
		IXvisioGrabbableInterface::Execute_OnGrabStart(Actor, HandType, nullptr);
	}

	OnGrabObject.Broadcast(HandType, Actor);

	UE_LOG(LogTemp, Log, TEXT("XvisioGrab: Grabbed %s with %s hand at pinch point"),
		*Actor->GetName(),
		HandType == EControllerHand::Right ? TEXT("Right") : TEXT("Left"));
}

void UXvisioHandGrabComponent::ReleaseObject()
{
	if (!GrabbedActor.IsValid())
	{
		return;
	}

	AActor* Actor = GrabbedActor.Get();
	GrabbedActor.Reset();

	// 恢复碰撞和物理
	UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(Actor->GetRootComponent());
	if (RootPrimitive)
	{
		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

		if (bSimulatePhysicsOnRelease)
		{
			RootPrimitive->SetSimulatePhysics(true);
			RootPrimitive->WakeAllRigidBodies();

			// 计算释放速度（上一帧到当前帧的位移），限制最大值
			FVector Velocity = (Actor->GetActorLocation() - PrevGrabbedLocation) / 0.016f; // ~60fps
			Velocity = Velocity.GetClampedToMaxSize(MaxReleaseVelocity);
			RootPrimitive->SetPhysicsLinearVelocity(Velocity);
		}
	}

	// 通知接口
	if (Actor->Implements<UXvisioGrabbableInterface>())
	{
		IXvisioGrabbableInterface::Execute_OnGrabRelease(Actor, HandType, bSimulatePhysicsOnRelease);
	}

	OnReleaseObject.Broadcast(HandType, Actor);

	UE_LOG(LogTemp, Log, TEXT("XvisioGrab: Released %s"), *Actor->GetName());
}

void UXvisioHandGrabComponent::UpdateGrabbedObject(const FVector& PinchPosition, const FQuat& PinchRotation)
{
	if (!GrabbedActor.IsValid())
	{
		return;
	}

	AActor* Actor = GrabbedActor.Get();

	// 直接设置物体位置 = 捏合点 + 偏移
	FVector NewLocation = PinchPosition + GrabOffsetLocation;
	Actor->SetActorLocation(NewLocation, false);

	// 旋转：用食指方向旋转带动物体（可选，保留初始旋转偏移）
	FQuat NewRotation = PinchRotation * GrabOffsetRotation;
	Actor->SetActorRotation(NewRotation);

	PrevGrabbedLocation = LastGrabbedLocation;
	LastGrabbedLocation = Actor->GetActorLocation();

	// 通知接口 tick
	if (Actor->Implements<UXvisioGrabbableInterface>())
	{
		IXvisioGrabbableInterface::Execute_OnGrabTick(Actor, HandType);
	}
}

bool UXvisioHandGrabComponent::IsGrabbable(AActor* Actor) const
{
	if (!Actor)
	{
		return false;
	}

	// 方式一：通过 C++ 接口
	if (Actor->Implements<UXvisioGrabbableInterface>())
	{
		return true;
	}

	// 方式二：通过 Actor 标签
	if (Actor->Tags.Contains(FName(TEXT("XvisioGrabbable"))))
	{
		return true;
	}

	// 方式三：检查根组件标签
	if (UActorComponent* RootComp = Actor->GetRootComponent())
	{
		if (RootComp->ComponentTags.Contains(FName(TEXT("XvisioGrabbable"))))
		{
			return true;
		}
	}

	return false;
}
