// Copyright (c) Yuquan Sun. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MREyeAnchoredUIPanel.generated.h"

class UUserWidget;
class UTextureRenderTarget2D;
class UMR3PanelWidget;
class FWidgetRenderer;

/**
 * 用 FaceLocked Stereo Layer（头锁立体层）把 UMG 面板始终钉在 HMD 眼前。
 *
 * 原理（UE 官方 VR UI 方案，绕开 World Space 定位的所有坑）：
 *   - 用 FWidgetRenderer 把 UMR3PanelWidget 渲染到 UTextureRenderTarget2D。
 *   - 通过 GEngine->StereoRenderingDevice->GetStereoLayers()（IStereoLayers）创建
 *     PositionType=FaceLocked 的层，把 RenderTarget 作为纹理交给 OpenXR runtime 合成。
 *   - FaceLocked 层由 XR runtime 直接渲染在 HMD 前方，跟随头部旋转，永远在眼前，
 *     完全不依赖 HMD position tracking（Xvisio 的 position tracking 有异常，不能用）。
 */
UCLASS(Blueprintable, ClassGroup = "MR3")
class MR3_API AEyeAnchoredUIPanel : public AActor
{
    GENERATED_BODY()

public:
    AEyeAnchoredUIPanel();

    /** UMG 类。要在编辑器里指定或在 BeginPlay 之前通过 SetWidgetClass 注入。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    TSubclassOf<UUserWidget> WidgetClass;

    /** UMG 内部分辨率。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    FVector2D WidgetDrawSize = FVector2D(1920.f, 1080.f);

    /** 面板距头的距离（cm）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI", meta = (ClampMin = "20", ClampMax = "500"))
    float Distance = 100.f;

    /** 面板物理尺寸（cm，宽 × 高）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    FVector2D QuadSize = FVector2D(50.f, 32.f);

    /** 面板垂直偏移（cm，+ = 上）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float VerticalOffset = -10.f;

    /** 手指射线停留在按钮上触发点击的时间（秒）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float HoverTriggerTime = 0.2f;

    /** 判定射线是否仍停留在同一位置的移动阈值（像素）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MR3|UI")
    float HoverMoveThreshold = 20.f;

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

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    void CreateStereoLayer();
    void UpdateHandInteraction();

    UPROPERTY(Transient)
    TObjectPtr<UTextureRenderTarget2D> PanelRT;

    UPROPERTY(Transient)
    TObjectPtr<UMR3PanelWidget> PanelWidget;

    FWidgetRenderer* WidgetRenderer = nullptr;
    uint32 StereoLayerId = 0;

    // 悬停触发状态
    bool bHovering = false;
    FVector2D LastHoverScreenPos = FVector2D::ZeroVector;
    float HoverStartTime = 0.f;

    // 缓存 SetSandboxRefs 的参数（widget 延迟创建，需要重试）
    TWeakObjectPtr<class AMRSandboxRoot> PendingSandbox;
    TWeakObjectPtr<class UMRHandUIInteractor> PendingInteractor;
    bool bSandboxRefsApplied = false;
};
