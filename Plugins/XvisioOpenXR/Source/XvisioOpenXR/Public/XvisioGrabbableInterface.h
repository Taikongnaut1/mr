#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "XvisioGrabbableInterface.generated.h"

UINTERFACE(BlueprintType)
class XVISIOOPENXR_API UXvisioGrabbableInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可抓取对象接口
 * C++ Actor 实现此接口来响应抓取事件；
 * 纯蓝图 Actor 可通过组件标签 "XvisioGrabbable" 识别。
 */
class XVISIOOPENXR_API IXvisioGrabbableInterface
{
	GENERATED_BODY()

public:
	/** 抓取开始（对手类型 + 附着的 SceneComponent） */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Xvisio|Grab")
	void OnGrabStart(EControllerHand Hand, USceneComponent* AttachComponent);

	/** 抓取中（每帧调用） */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Xvisio|Grab")
	void OnGrabTick(EControllerHand Hand);

	/** 抓取释放 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Xvisio|Grab")
	void OnGrabRelease(EControllerHand Hand, bool bSimulatePhysics);
};
