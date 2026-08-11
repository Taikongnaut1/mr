#include "XvisioLocomotionComponent.h"

#include "UXVisioXRWorldSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

UXvisioLocomotionComponent::UXvisioLocomotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UXvisioLocomotionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 缓存相机组件
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		CachedCamera = OwnerPawn->FindComponentByClass<UCameraComponent>();
	}

	// 获取 WorldSubsystem 并绑定事件
	UWorld* World = GetWorld();
	if (World)
	{
		WorldSubsystem = World->GetSubsystem<UXVisioXRWorldSubsystem>();
	}

	SetupDefaultKeyMappings();
	BindXvisioEvents();
}

void UXvisioLocomotionComponent::SetupDefaultKeyMappings()
{
	// 默认按键码映射（需根据实机测试调整）
	// 假设 Xvisio 控制器四个方向键分别对应 eventType 1-5
	if (KeyCodeDirectionMap.Num() == 0)
	{
		KeyCodeDirectionMap.Add(1, EXvisioDirection::Forward);
		KeyCodeDirectionMap.Add(2, EXvisioDirection::Backward);
		KeyCodeDirectionMap.Add(4, EXvisioDirection::Left);
		KeyCodeDirectionMap.Add(5, EXvisioDirection::Right);
	}
}

void UXvisioLocomotionComponent::BindXvisioEvents()
{
	if (!WorldSubsystem || !bEnableControllerMovement)
	{
		return;
	}

	KeyDownHandle = WorldSubsystem->OnXvisioKeyDown.AddUObject(
		this, &UXvisioLocomotionComponent::HandleXvisioKeyDown);
	KeyUpHandle = WorldSubsystem->OnXvisioKeyUp.AddUObject(
		this, &UXvisioLocomotionComponent::HandleXvisioKeyUp);
}

void UXvisioLocomotionComponent::UnbindXvisioEvents()
{
	if (WorldSubsystem)
	{
		if (KeyDownHandle.IsValid())
		{
			WorldSubsystem->OnXvisioKeyDown.Remove(KeyDownHandle);
			KeyDownHandle.Reset();
		}
		if (KeyUpHandle.IsValid())
		{
			WorldSubsystem->OnXvisioKeyUp.Remove(KeyUpHandle);
			KeyUpHandle.Reset();
		}
	}
}

void UXvisioLocomotionComponent::HandleXvisioKeyDown(int32 KeyCode)
{
	EXvisioDirection* Dir = KeyCodeDirectionMap.Find(KeyCode);
	if (Dir)
	{
		SetDirectionState(*Dir, true);
	}
}

void UXvisioLocomotionComponent::HandleXvisioKeyUp(int32 KeyCode)
{
	EXvisioDirection* Dir = KeyCodeDirectionMap.Find(KeyCode);
	if (Dir)
	{
		SetDirectionState(*Dir, false);
	}
}

void UXvisioLocomotionComponent::SetDirectionState(EXvisioDirection Direction, bool bPressed)
{
	switch (Direction)
	{
	case EXvisioDirection::Forward:  bForwardPressed  = bPressed; break;
	case EXvisioDirection::Backward: bBackwardPressed = bPressed; break;
	case EXvisioDirection::Left:     bLeftPressed     = bPressed; break;
	case EXvisioDirection::Right:    bRightPressed    = bPressed; break;
	}
}


void UXvisioLocomotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 构建移动输入向量
	CurrentMovementInput = FVector2D::ZeroVector;

	// 键盘输入（每帧清零后用键盘覆盖）
	bool bKbdForward = false, bKbdBackward = false, bKbdLeft = false, bKbdRight = false;
	if (bEnableKeyboardMovement)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			bKbdForward  = PC->IsInputKeyDown(EKeys::W);
			bKbdBackward = PC->IsInputKeyDown(EKeys::S);
			bKbdLeft     = PC->IsInputKeyDown(EKeys::A);
			bKbdRight    = PC->IsInputKeyDown(EKeys::D);
		}
	}

	float ForwardInput = 0.0f;
	float RightInput = 0.0f;

	if (bForwardPressed || bKbdForward)
		ForwardInput += 1.0f;
	if (bBackwardPressed || bKbdBackward)
		ForwardInput -= 1.0f;
	if (bRightPressed || bKbdRight)
		RightInput += 1.0f;
	if (bLeftPressed || bKbdLeft)
		RightInput -= 1.0f;

	CurrentMovementInput = FVector2D(ForwardInput, RightInput);

	// 应用移动
	if (!CurrentMovementInput.IsNearlyZero())
	{
		ApplyMovement(DeltaTime);
	}
}

FVector UXvisioLocomotionComponent::GetHMDForwardDirection() const
{
	// 优先使用缓存的相机
	if (CachedCamera)
	{
		FRotator CameraRot = CachedCamera->GetComponentRotation();
		// 只取 Yaw（忽略 Pitch/Roll），确保水平移动
		return FRotationMatrix(FRotator(0.0f, CameraRot.Yaw, 0.0f)).GetUnitAxis(EAxis::X);
	}

	// Fallback: 使用 Pawn 朝向
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (OwnerPawn)
	{
		FRotator PawnRot = OwnerPawn->GetActorRotation();
		return FRotationMatrix(FRotator(0.0f, PawnRot.Yaw, 0.0f)).GetUnitAxis(EAxis::X);
	}

	return FVector::ForwardVector;
}

void UXvisioLocomotionComponent::ApplyMovement(float DeltaTime)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return;
	}

	FVector HMDForward = GetHMDForwardDirection();
	FVector HMDRight = HMDForward.RotateAngleAxis(90.0f, FVector::UpVector);

	// 合成移动方向
	FVector MoveDirection = HMDForward * CurrentMovementInput.X + HMDRight * CurrentMovementInput.Y;
	MoveDirection.Normalize();

	FVector Movement = MoveDirection * MoveSpeed * DeltaTime;

	// 尝试使用 AddMovementInput（适用于 ACharacter）
	ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerPawn);
	if (OwnerCharacter && OwnerCharacter->GetCharacterMovement())
	{
		float Scale = CurrentMovementInput.Size();
		OwnerPawn->AddMovementInput(MoveDirection, FMath::Min(Scale, 1.0f));
	}
	else
	{
		// 直接移动 Pawn（适用于没有 CharacterMovementComponent 的情况）
		FVector NewLocation = OwnerPawn->GetActorLocation() + Movement;
		OwnerPawn->SetActorLocation(NewLocation, true);
	}

	// Debug 绘制
	if (bDrawDebug)
	{
		UKismetSystemLibrary::DrawDebugArrow(
			GetWorld(),
			OwnerPawn->GetActorLocation(),
			OwnerPawn->GetActorLocation() + MoveDirection * 100.0f,
			10.0f,
			FLinearColor::Blue,
			0.0f,
			3.0f);
	}
}
