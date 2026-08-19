// Copyright (c) Yuquan Sun. All rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MRSandboxRoot.generated.h"

class ULightComponent;
class UMRHandUIInteractor;

UCLASS()
class MR3_API AMRSandboxRoot : public AActor
{
    GENERATED_BODY()

public:
    AMRSandboxRoot();

    /** Auto-spawn into any newly initialized Game/PIE world (no manual placement needed). */
    static void OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS);

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Actors to be attached as children of this sandbox root. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    TArray<AActor*> ChildActors;

    /** Initial world scale of the sandbox (0.1 = 1/10, applied automatically on BeginPlay). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float InitialScale = 0.1f;

    /** Vertical offset of the sandbox centre relative to the camera (cm). Negative = lower. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "-300", ClampMax = "300"))
    float SandboxHeightOffset = -80.0f;

    /** Distance in front of the camera where the sandbox centre sits (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox", meta = (ClampMin = "50", ClampMax = "1000"))
    float SandboxForwardDistance = 300.0f;

    /** Initial offset in front of the player (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    FVector InitialOffset = FVector(100.0f, 0.0f, 0.0f);

    /** Height above the detected plane / floor (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float HoverHeight = 10.0f;

    /** Scale sensitivity for two-hand pinch. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float ScaleSensitivity = 5.0f;

    /** Rotation sensitivity for two-hand pinch (degrees per degree). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float RotationSensitivity = 5.0f;

    /** Translation sensitivity for single-hand grab (cm per cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float TranslationSensitivity = 5.0f;

    /** Thumb-to-index distance below this value is considered a pinch (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float PinchThreshold = 2.0f;

    /** At least 3 of 4 fingertips closer than this to palm is considered a grab (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float GrabThreshold = 9.0f;

    /** Minimum allowed sandbox scale. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float MinScale = 0.5f;

    /** Maximum allowed sandbox scale. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    float MaxScale = 3.0f;

    /** Actors visible only in stage 1. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    TArray<AActor*> Stage1Actors;

    /** Actors visible only in stage 2. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    TArray<AActor*> Stage2Actors;

    /** Actors visible only in stage 3. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    TArray<AActor*> Stage3Actors;

    /** Actors visible only in stage 4. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sandbox")
    TArray<AActor*> Stage4Actors;

    /** Current stage index (1-4). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sandbox")
    int32 CurrentStage = 1;

    /** The actor currently aimed at by the index finger ray. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Sandbox")
    AActor* AimedActor = nullptr;

    /** Reset sandbox to initial transform and scale. */
    UFUNCTION(BlueprintCallable, Category = "Sandbox")
    void ResetSandbox();

    /** Switch to the given stage (1-4). */
    UFUNCTION(BlueprintCallable, Category = "Sandbox")
    void SetStage(int32 Stage);

    /** 暂停 / 继续 当前阶段动画。让蓝图菜单的 ▶ / ⏸ 按钮直接调。 */
    UFUNCTION(BlueprintCallable, Category = "Sandbox|Animation")
    void SetAnimationPaused(bool bPaused);

    UFUNCTION(BlueprintPure, Category = "Sandbox|Animation")
    bool IsAnimationPaused() const { return bAnimationPaused; }

    /** 切换泄漏源（fire_cue + molten_metal_plane + molten_metal_flow）的显示/隐藏。 */
    UFUNCTION(BlueprintCallable, Category = "Sandbox|Leak")
    void ToggleLeakVisibility();

    /** 高亮泄漏源（发光橙色）。 */
    UFUNCTION(BlueprintCallable, Category = "Sandbox|Leak")
    void HighlightLeak();

    /**
     * 让蓝图把 UMRHandUIInteractor 挂上来。Tick 中会把
     * OpenXR/Xvisio 输出的右手食指 transform 每帧喂给它。
     * 注意：Interactor 内部已经包了 UWidgetInteractionComponent。
     */
    UFUNCTION(BlueprintCallable, Category = "Sandbox|UI")
    void RegisterHandUIInteractor(class UMRHandUIInteractor* InInteractor);

    /** [Editor only] Collect all factory-relevant actors in the current level into ChildActors. */
    UFUNCTION(CallInEditor, Category = "Sandbox")
    void CollectFactoryActorsFromLevel();

    /** [Editor only] Attach all ChildActors to this root and apply InitialScale. */
    UFUNCTION(CallInEditor, Category = "Sandbox")
    void AttachChildrenInEditor();

    /** [Editor only] Clear the ChildActors array. */
    UFUNCTION(CallInEditor, Category = "Sandbox")
    void ClearChildActors();

    /** Scale the sandbox by a relative delta (clamped to MinScale..MaxScale * InitialScale). */
    UFUNCTION(BlueprintCallable, Category = "Sandbox")
    void ScaleSandbox(float DeltaScale);

    /** Rotate the sandbox around the vertical axis by DeltaYaw degrees. */
    UFUNCTION(BlueprintCallable, Category = "Sandbox")
    void RotateSandbox(float DeltaYaw);

    /** Translate the sandbox in the horizontal plane by Delta (cm). */
    UFUNCTION(BlueprintCallable, Category = "Sandbox")
    void TranslateSandboxXY(const FVector2D& Delta);

    /** Scale all PointLight/SpotLight attenuation radii by current actor scale (caches originals on first call). */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Sandbox|Lights")
    void AdjustLightsForScale();

    /** Restore cached light attenuation radii to their original values. */
    UFUNCTION(CallInEditor, BlueprintCallable, Category = "Sandbox|Lights")
    void RestoreLightRadii();

protected:
    void ApplySandboxScale();
    void UpdateHandInteraction(float DeltaTime);
    void UpdateStageVisibility();
    /** 把手部追踪的食指 transform 喂给蓝图注册上来的 UMRHandUIInteractor。 */
    void FeedHandUITransform();
    bool IsIndexExtended(EControllerHand Hand);

    bool GetHandPinchAndGrab(EControllerHand Hand, FVector& OutPinchPoint, bool& bOutPinching, bool& bOutGrabbing);

    // --- 动画控制 ---
    bool bAnimationPaused = false;

    // --- 动画系统（4 个按键对应 4 段动画）---
    enum class ESandboxAnimation : uint8
    {
        None = 0,
        Stage1_Leak,       // 阶段一：泄漏与避险——工人远离
        Stage2_Scan,       // 阶段二：侦察与指挥——无人机扫描
        Stage3_Extinguish, // 阶段三：封控与处置——消防车/机器人推进
        Stage4_Medical     // 阶段四：医疗——医生靠近
    };
    ESandboxAnimation CurrentAnimation = ESandboxAnimation::None;
    float AnimationTime = 0.0f;
    void UpdateAnimation(float DeltaTime);

    // 给 Actor 的所有 StaticMesh 设置颜色（通过 MID）
    void SetActorColor(AActor* A, const FLinearColor& Color);

    // 泄漏相关 Actor（fire_cue + Plane + MoltenFlow）
    TArray<AActor*> LeakActors;
    TArray<FVector> LeakOriginalScales; // 缓存原始缩放
    void InitLeakActors();

    // 按形状分类的 Actor（用于精准动画）
    TArray<AActor*> CylinderActors;  // 圆柱 = 高炉（不含金属流）
    TArray<AActor*> CapsuleActors;   // 胶囊 = 人物
    TArray<AActor*> FactoryActors;   // 厂房（changfang）
    TArray<AActor*> FiretruckActors; // 消防车（xiaofang）
    TArray<AActor*> SphereActors;    // 圆球 = 无人机
    TArray<AActor*> PlaneActors;     // 平面 = 泄漏金属平面
    TArray<AActor*> MoltenFlowActors; // 熔融金属流
    void ClassifyActorsByShape();

    // Stage 3 水雾粒子状态（成员变量，避免函数内 static 跨实例共享）
    class UParticleSystem* SteamPS = nullptr;
    float LastSteamTime = 0.0f;

    // 泄漏源当前可见状态
    bool bLeakVisible = false;

    FTransform InitialTransform;

    bool bWasBothPinching = false;
    bool bWasGrabbing = false;

    float PreviousSpan = 0.0f;
    float PreviousYaw = 0.0f;
    FVector PreviousGrabPoint = FVector::ZeroVector;

    /** Grace counter: when grab tracking flickers (1/0 frame-to-frame), keep PreviousGrabPoint
     *  for up to N frames so Delta doesn't reset to zero on every flicker. */
    int32 GrabGraceFrames = 0;

    /** Hysteresis state for left/right grab — avoids frame-to-frame flicker.
     *  Trigger grab: 3+ fingers curled. Release grab: 1 or fewer fingers curled. */
    bool bLastLeftGrab = false;
    bool bLastRightGrab = false;

    /** Cached original transforms of ChildActors (used by ApplySandboxScale to scale each actor in place). */
    TArray<FTransform> OriginalTransforms;

    /** Geometric centroid of all ChildActors in their original world positions (captured at BeginPlay). */
    FVector SandboxSceneCenter = FVector::ZeroVector;

    /** Where the sandbox's scene centre should be placed after scaling (in front of the player). */
    FVector SandboxTargetCenter = FVector::ZeroVector;

    /** Current uniform scale multiplier applied to ChildActors (Start = InitialScale; gestures adjust this). */
    float CurrentScale = 0.1f;

    /** Current yaw rotation of the sandbox (degrees, around vertical axis). Gestures/keyboard adjust this. */
    float SandboxYaw = 0.0f;

    /** Current horizontal translation of the sandbox (cm). Single-hand grab adjusts this. */
    FVector SandboxTranslation = FVector::ZeroVector;

    /** Last known index-fingertip position (for drawing the aim ray in Tick). */
    FVector LastIndexTipPos = FVector::ZeroVector;

    // --- PC debug input (works without HMD, for editor/PIE testing) ---
    void SetupPCDebugInput();
    UFUNCTION() void OnScrollUp();
    UFUNCTION() void OnScrollDown();
    UFUNCTION() void OnRMBDown();
    UFUNCTION() void OnRMBUp();
    bool bRMBDown = false;
    FVector2D LastMousePos = FVector2D::ZeroVector;
    UFUNCTION() void OnKey1();
    UFUNCTION() void OnKey2();
    UFUNCTION() void OnKey3();
    UFUNCTION() void OnKey4();
    UFUNCTION() void OnKeyR();
    UFUNCTION() void OnKeyQ();
    UFUNCTION() void OnKeyE();

    // --- Light attenuation cache (scaled independently because light radii do not follow actor scale) ---
    struct FLightCache
    {
        TWeakObjectPtr<ULightComponent> Comp;
        float OriginalRadius = 0.0f;
    };
    TArray<FLightCache> CachedLights;
    bool bLightsCached = false;

    static constexpr float ScrollScaleStep = 0.05f;
    static constexpr float KeyRotateStep = 2.0f;

    /** 由蓝图菜单挂上来的"食指→UMG事件"桥。Tick 中把食指 transform 喂给它。 */
    TWeakObjectPtr<class UMRHandUIInteractor> HandUIInteractor;
};
