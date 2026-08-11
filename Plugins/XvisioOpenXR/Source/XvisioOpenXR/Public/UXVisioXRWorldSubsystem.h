#pragma once
#include "Standalone/FXvisioFunctionPlugin.h"
#include "Standalone/KeyEventRunable.h"
#include "Standalone/XvisioOpenXR.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "IXRTrackingSystem.h"
#include "UXVisioXRWorldSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyDownBP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyUpBP);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnKeyDownTickBP);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnXvisioKeyEventNative, int32);

/** Xvisio 按键事件数据（蓝图层使用） */
USTRUCT(BlueprintType)
struct XVISIOOPENXR_API FXvisioKeyEventData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Xvisio|Input")
	int32 EventType = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Xvisio|Input")
	int32 EventState = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Xvisio|Input")
	int32 KeyCode = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Xvisio|Input")
	double HostTimestamp = 0.0;
};

UCLASS()
class XVISIOOPENXR_API UXVisioXRWorldSubsystem
	: public UWorldSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Input")
	FOnKeyDownBP OnKeyDown;
	
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Input")
	FOnKeyDownTickBP OnKeyDownTick;
	
	UPROPERTY(BlueprintAssignable, Category = "Xvisio|Input")
	FOnKeyUpBP OnKeyup;

	unsigned char  TagFamily[6] = {'3','6','h','1','1',0};
	double TagSize = 0.16;

	XrAprilTagDataXV tagsArray[32]; // 最大支持32个tag
	uint32_t TagCout;
	int32 MaxArraySize = 32;
	int32 DetectType = 1;
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool StartEvent();
	
	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool StopEvent();
	
	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool testEvent();

	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool GetApriltag(const FString& InTagFamily, float InTagSize, int32 InDetectType, int32& OutTagCount);

	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool GetApriltagData(int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence);

	UFUNCTION(BlueprintCallable, Category = "Xvisio")
	bool StopApriltag(int ret);

	// ========== 扩展按键事件（带方向信息）==========

	/** C++ 层按键事件（带 KeyCode，供组件内部使用） */
	FOnXvisioKeyEventNative OnXvisioKeyDown;
	FOnXvisioKeyEventNative OnXvisioKeyUp;
	FOnXvisioKeyEventNative OnXvisioKeyTick;

	/** 查询最后一次事件数据 */
	FXvisioKeyEventData GetLastKeyEventData() const { return LastKeyEventData; }

	/** 查询指定 KeyCode 是否被按下 */
	bool IsKeyPressed(int32 KeyCode) const { return PressedKeys.Contains(KeyCode); }
	
	uint32_t Brightness = 50;
private:
	bool isTickable = false;
	XvisioOpenXR::FXVisioOpenXRModule* xVisioOpenXRModule = nullptr;
	XvisioOpenXR::FKeyEventPlugin* keyEventPlugin = nullptr;
	XvisioOpenXR::AKeyEventRunable* keyEventRunable = nullptr;

	XvisioOpenXR::FXvisioFunctionPlugin* XvisioFunctionPlugin = nullptr;

	IXRTrackingSystem* XRTrackingSystem = nullptr;
	
	FXvisioKeyEventData LastKeyEventData;
	TSet<int32> PressedKeys;

	// 方向键事件绑定句柄
	FDelegateHandle Dir1DownHandle;
	FDelegateHandle Dir1UpHandle;
	FDelegateHandle Dir2DownHandle;
	FDelegateHandle Dir2UpHandle;
	FDelegateHandle Dir3DownHandle;
	FDelegateHandle Dir3UpHandle;
	FDelegateHandle Dir4DownHandle;
	FDelegateHandle Dir4UpHandle;

	void HandleNativeKeyDown();
	
	void HandleNativeKeyUp();
	
	void HandleNativeOnKeyDownTick();
	
};
