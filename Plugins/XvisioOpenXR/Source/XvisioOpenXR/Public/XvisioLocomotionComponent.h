#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XvisioLocomotionComponent.generated.h"

/** Xvisio 方向枚举（用于按键码映射） */
UENUM(BlueprintType)
enum class EXvisioDirection : uint8
{
	Forward  UMETA(DisplayName = "前进"),
	Backward UMETA(DisplayName = "后退"),
	Left     UMETA(DisplayName = "左移"),
	Right    UMETA(DisplayName = "右移")
};

/**
 * Xvisio 平滑移动组件
 * 支持 Xvisio 控制器按键 + WASD 键盘实现基于 HMD 朝向的平滑移动。
 */
UCLASS(ClassGroup = (Xvisio), Blueprintable, BlueprintType, meta = (BlueprintSpawnableComponent))
class XVISIOOPENXR_API UXvisioLocomotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXvisioLocomotionComponent();

	// ========== 配置参数 ==========

	/** 移动速度 (cm/s) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Locomotion", meta = (ClampMin = "10.0", ClampMax = "2000.0"))
	float MoveSpeed = 300.0f;

	/** 是否启用 Xvisio 控制器按键移动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Locomotion")
	bool bEnableControllerMovement = true;

	/** 是否启用 WASD 键盘移动 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Locomotion")
	bool bEnableKeyboardMovement = true;

	/** Debug 绘制开关 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Locomotion|Debug")
	bool bDrawDebug = false;

	// ========== 按键码映射（可在蓝图中配置） ==========

	/** Xvisio 按键码 → 移动方向 的映射表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Xvisio|Locomotion|Keys")
	TMap<int32, EXvisioDirection> KeyCodeDirectionMap;

	// ========== 蓝图层 ==========

	/** 获取当前移动方向（世界空间 XY） */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Locomotion")
	FVector2D GetCurrentMovementDirection() const { return CurrentMovementInput; }

	/** 获取移动速度 */
	UFUNCTION(BlueprintPure, Category = "Xvisio|Locomotion")
	float GetCurrentMoveSpeed() const { return MoveSpeed; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ========== 状态 ==========
	FVector2D CurrentMovementInput;
	bool bForwardPressed  = false;
	bool bBackwardPressed = false;
	bool bLeftPressed     = false;
	bool bRightPressed    = false;

	class UXVisioXRWorldSubsystem* WorldSubsystem = nullptr;
	class UCameraComponent* CachedCamera = nullptr;

	FDelegateHandle KeyDownHandle;
	FDelegateHandle KeyUpHandle;

	// ========== 内部方法 ==========
	void SetupDefaultKeyMappings();
	void BindXvisioEvents();
	void UnbindXvisioEvents();
	void HandleXvisioKeyDown(int32 KeyCode);
	void HandleXvisioKeyUp(int32 KeyCode);
	FVector GetHMDForwardDirection() const;
	void ApplyMovement(float DeltaTime);
	void SetDirectionState(EXvisioDirection Direction, bool bPressed);
};
