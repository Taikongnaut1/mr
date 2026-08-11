// Copyright (c) Yuquan Sun. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MREyeAnchoredUIPanel.generated.h"

class UWidgetComponent;
class UCameraComponent;
class UUserWidget;

/**
 * 把一个 UMG Widget 钉到玩家眼前固定距离、始终面向相机的容器 Actor。
 *
 * 设计要点：
 *   - WidgetComponent 的 Space=World；在 Tick(PostUpdateWork) 中每帧把 Actor
 *     的位置/旋转同步到 HMD Camera 局部坐标系里的 (Fwd*Distance + Right*HRight + Up*VUp)。
 *   - 这样面板始终在视野正中偏下，处于 "一直位于眼前" 的状态，且缩放不限 ——
 *     你就是走到哪里它都跟着。
 *   - 不依赖 OpenXR。只需要工程里有任何来源的 CameraComponent（HMD、PlayerCameraManager 都行）。
 */
UCLASS(Blueprintable, ClassGroup = "MR3")
class MR3_API AEyeAnchoredUIPanel : public AActor
{
    GENERATED_BODY()

public:
    AEyeAnchoredUIPanel();

    /** 内部渲染组件（UMG 容器）。 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MR3|UI")
    TObjectPtr<UWidgetComponent> WidgetComp;

    /** UMG 类。要在编辑器里指定或在 BeginPlay 之前通过 SetWidgetClass 注入。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    TSubclassOf<UUserWidget> WidgetClass;

    /** UMG 内部分辨率（DPI 大约 200 时 1920x1080 对应 ~24cm×13.5cm 物理尺寸）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    FVector2D WidgetDrawSize = FVector2D(1920.f, 1080.f);

    /** 面板距头的距离（cm）。Quest 推荐 80~120cm。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI", meta = (ClampMin = "20", ClampMax = "500"))
    float Distance = 80.f;

    /** 面板水平偏移（cm，相对于相机右轴；+ = 右）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float HorizontalOffset = 25.f;

    /** 面板垂直偏移（cm，相对于相机上轴；+ = 上；通常给 -12~-20 避开 HMD 视野中心）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float VerticalOffset = -5.f;

    /** 手动指定锚点相机；为空则自动取 PlayerCameraManager 视点相机或 Pawn 上的第一个 CameraComponent。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    TObjectPtr<UCameraComponent> OverrideCamera;

    /** 运行时换 UMG 类。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetWidgetClass(TSubclassOf<UUserWidget> InClass);

    /** 鼠标调试：PIE 中用鼠标替代手部射线。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetMouseDebugMode(bool bEnabled);

    /** 设置要转发命令的沙盘根对象和手部交互器。 */
    UFUNCTION(BlueprintCallable, Category = "MR3|UI")
    void SetSandboxRefs(class AMRSandboxRoot* InSandbox, class UMRHandUIInteractor* InInteractor);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    bool bMouseDebugMode = false;

    /** Screen 模式（PIE 调试用，把 UI 钉在屏幕前面）vs World 模式（HMD/MR 用，UI 钉在世界空间里）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    bool bUseScreenSpace = false;

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

private:
    void TryBindCamera();

    UPROPERTY(Transient)
    TWeakObjectPtr<UCameraComponent> CachedCamera;
};
