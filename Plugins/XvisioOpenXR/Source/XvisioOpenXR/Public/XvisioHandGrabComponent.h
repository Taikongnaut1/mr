#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "XvisioHandGrabComponent.generated.h"

/**
 * 手部捏合抓取组件
 * 使用 XvisioOpenXR 手部关节数据检测捏合手势，
 * 自动查找并抓取前方可交互物体。
 */
UCLASS(ClassGroup = (Xvisio), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class XVISIOOPENXR_API UXvisioHandGrabComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXvisioHandGrabComponent();

	// ========== 配置参数 ==========

	/** 哪只手 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab")
	EControllerHand HandType = EControllerHand::Right;

	/** 捏合距离阈值 (cm) —— 拇指尖与食指尖距离小于此值判定为捏合 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float PinchThreshold = 4.5f;

	/** 抓取检测半径 (cm) —— 以捏合点为中心的球体检测范围 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float GrabRadius = 12.0f;

	/** 最近筛选：最多同时检测多少个候选对象 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab")
	int32 MaxGrabCandidates = 10;

	/** 释放时是否开启物理模拟 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab")
	bool bSimulatePhysicsOnRelease = true;

	/** 释放时最大速度 (cm/s) —— 限制甩飞速度，利于堆叠稳定 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float MaxReleaseVelocity = 30.0f;

	/** 手离 HMD 小于此距离 (cm) 时不触发抓取，避免遮挡时数据不稳 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab", meta = (ClampMin = "0.0", ClampMax = "50.0"))
	float MinHMDDistance = 10.0f;

	/** Debug 绘制开关 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Grab|Debug")
	bool bDrawDebug = false;

	// ========== 蓝图层查询 ==========

	/** 当前是否正在捏合 */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Grab")
	bool IsPinching() const { return bIsPinching; }

	/** 当前是否正在抓取物体 */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Grab")
	bool IsGrabbing() const { return GrabbedActor.IsValid(); }

	/** 获取当前抓取的 Actor */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Grab")
	AActor* GetGrabbedActor() const { return GrabbedActor.Get(); }

	/** 获取捏合位置（拇指与食指中点，世界空间） */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Grab")
	FVector GetPinchLocation() const { return LastPinchLocation; }

	// ========== 蓝图事件委托 ==========

	/** 捏合开始 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPinchStart, EControllerHand, Hand);
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Grab")
	FOnPinchStart OnPinchStart;

	/** 捏合结束 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPinchEnd, EControllerHand, Hand);
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Grab")
	FOnPinchEnd OnPinchEnd;

	/** 抓取物体 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGrabObject, EControllerHand, Hand, AActor*, GrabbedObject);
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Grab")
	FOnGrabObject OnGrabObject;

	/** 释放物体 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnReleaseObject, EControllerHand, Hand, AActor*, ReleasedObject);
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Grab")
	FOnReleaseObject OnReleaseObject;

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ========== 状态 ==========
	bool bIsPinching = false;
	bool bWasPinching = false;
	FVector LastPinchLocation = FVector::ZeroVector;
	TWeakObjectPtr<AActor> GrabbedActor;
	FVector GrabOffsetLocation = FVector::ZeroVector;  // 抓取时物体与捏合点的偏移
	FQuat GrabOffsetRotation = FQuat::Identity;        // 抓取时物体与捏合点旋转的偏移
	FVector LastGrabbedLocation = FVector::ZeroVector;
	FVector PrevGrabbedLocation = FVector::ZeroVector; // 上一帧位置（用于计算释放速度）

	/** 标准 OpenXR 手部关节索引 */
	static constexpr int32 ThumbTipJointIndex  = 5;   // XR_HAND_JOINT_THUMB_TIP_EXT
	static constexpr int32 IndexTipJointIndex   = 10;  // XR_HAND_JOINT_INDEX_TIP_EXT
	static constexpr int32 IndexDistalJointIndex = 9;  // XR_HAND_JOINT_INDEX_DISTAL_EXT（方向参考）
	static constexpr int32 PalmJointIndex       = 0;   // XR_HAND_JOINT_PALM_EXT

	// ========== 内部方法 ==========
	bool CheckPinchGesture(const TArray<FVector>& Positions);
	FVector GetPinchPoint(const TArray<FVector>& Positions) const;
	bool IsHandTooCloseToHMD(const FVector& HandPosition) const;
	AActor* FindNearestGrabbable(const FVector& PinchPosition);
	void GrabObject(AActor* Actor, const FVector& PinchPosition);
	void ReleaseObject();
	void UpdateGrabbedObject(const FVector& PinchPosition, const FQuat& PinchRotation);
	bool IsGrabbable(AActor* Actor) const;
};
