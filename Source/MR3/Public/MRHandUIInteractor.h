// Copyright (c) Yuquan Sun. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MRHandUIInteractor.generated.h"

class UWidgetInteractionComponent;
class USceneComponent;

/**
 * 把食指尖端的 transform 当作 "虚拟指针" 喂给 UWidgetInteractionComponent。
 *
 * 使用方式（两种皆可）：
 *   A) 在 Pawn 上挂这个组件，然后在外部每帧：
 *        Interactor->SetHandTransform( IndexTipTransform );
 *      （IndexTipTransform 由你的 OpenXR/Xvisio 手部追踪每帧给出）
 *      按 Pinch 时调 SetPinchPressed(true)，松开时 false。
 *
 *   B) 把 Owner 上的某个子组件作为索引 0（左手）/1（右手），用 InitializeWithHandSceneComponents，
 *      它会自动每帧复制该子组件的世界 transform 给 Interaction。
 *
 * UWidgetInteractionComponent 自身不需要 OpenXR，它只是个把世界射线
 * 转换成 UMG 指针事件的桥。它可以是源自 World/LeftHand/RightHand/Custom。
 * 选 World 后，"虚拟指针" 直接由 SetComponentTransform 控制。
 */
UCLASS(ClassGroup = "MR3", meta = (BlueprintSpawnableComponent))
class MR3_API UMRHandUIInteractor : public UActorComponent
{
    GENERATED_BODY()

public:
    UMRHandUIInteractor();

    /** 内嵌的交互组件。可在编辑器里微调 InteractionDistance、TraceChannel 等。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR3|UI")
    TObjectPtr<UWidgetInteractionComponent> Interaction;

    /** 自动跟踪源（可选）：左手 / 右手 上挂的追踪根组件（OpenXR 通常每帧写它）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    TObjectPtr<USceneComponent> HandTrackingRoot;

    /** Tick 时自动把 HandTrackingRoot 的世界 transform 复制给 Interaction。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    bool bAutoTrackHandRoot = true;

    /** InteractionDistance（cm）。建议比 UI 距离大一截，留点击余量。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float InteractionDistance = 250.f;

    /** 调试：画射线，便于在 PIE 里调位置。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    bool bDebugDraw = false;

    /** 手动设定手部 transform（auto-track 也可）。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetHandTransform(const FTransform& NewTransform);

    /** Pinch 状态变化：true=按下 LMB，false=释放 LMB。UMG 按钮的 OnClicked 由这个驱动。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetPinchPressed(bool bPressed);

    /** 当前 Pinch 状态。 */
    UFUNCTION(BlueprintPure, Category = "MR3|UI")
    bool IsPinchPressed() const { return bLastPressed; }

    /** 让 Interaction 自动跟随 HandTrackingRoot。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void InitializeWithHandSceneComponents(USceneComponent* InHandRoot);

    /** 鼠标调试模式：从 PlayerController 获取屏幕鼠标 → Deproject → 用射线模拟手指。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    bool bMouseDebugMode = false;

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
    bool bLastPressed = false;
    void ApplyMouseDebugRay();
};
