// Copyright (c) Yuquan Sun. All rights reserved.

#include "MRSandboxRoot.h"
#include "Standalone/XvisioOpenXR.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Components/InputComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Particles/ParticleSystem.h"
#include "MREyeAnchoredUIPanel.h"
#include "MRHandUIInteractor.h"
#include "MR3PanelWidget.h"
#include "Blueprint/UserWidget.h"
#include "IHandTracker.h"
#include "Features/IModularFeatures.h"
#include "IXRTrackingSystem.h"
#include "Engine/Engine.h"
#include "CoreGlobals.h"

namespace MRSandboxJoints
{
    static constexpr int32 Palm = 0;
    static constexpr int32 ThumbTip = 5;
    static constexpr int32 IndexTip = 10;
    static constexpr int32 MiddleTip = 15;
    static constexpr int32 RingTip = 20;
    static constexpr int32 LittleTip = 25;
}

// Helper: fetch the active IHandTracker (OpenXR native, same source as BP_TrackedHands).
// Returns null if no hand tracker feature is registered.
static IHandTracker* GetActiveHandTracker()
{
    if (!IModularFeatures::Get().IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
    {
        return nullptr;
    }
    return &IModularFeatures::Get().GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
}

AMRSandboxRoot::AMRSandboxRoot()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMRSandboxRoot::BeginPlay()
{
    Super::BeginPlay();

    FVector Location = GetActorLocation();
    FRotator Rotation = GetActorRotation();

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (PC && PC->GetPawn())
    {
        const FVector PawnLoc = PC->GetPawn()->GetActorLocation();
        const FVector Forward = PC->GetPawn()->GetActorForwardVector();
        const FVector Right = PC->GetPawn()->GetActorRightVector();

        Location = PawnLoc
            + Forward * InitialOffset.X
            + Right * InitialOffset.Y
            + FVector::UpVector * (70.0f + HoverHeight);

        Rotation.Yaw = PC->GetPawn()->GetActorRotation().Yaw;
    }

    SetActorLocation(Location);
    SetActorRotation(Rotation);
    SetActorScale3D(FVector(InitialScale));

    InitialTransform = GetActorTransform();

    if (ChildActors.Num() == 0)
    {
        CollectFactoryActorsFromLevel();
    }

    // Cache original transforms (we scale each ChildActor in place to avoid Static->Movable Attach errors).
    OriginalTransforms.Empty(ChildActors.Num());
    int32 ValidCount = 0;
    SandboxSceneCenter = FVector::ZeroVector;
    for (AActor* Child : ChildActors)
    {
        const FTransform T = Child ? Child->GetActorTransform() : FTransform::Identity;
        OriginalTransforms.Add(T);
        if (Child)
        {
            SandboxSceneCenter += T.GetLocation();
            ++ValidCount;
        }
    }
    if (ValidCount > 0)
    {
        SandboxSceneCenter /= ValidCount;
    }

    // Target centre: where the sandbox scene-centre should sit after scaling.
    // Use PlayerViewPoint (camera) as the most reliable anchor — GetPawn() may still be null
    // at BeginPlay in Standalone PIE. Fallback to world origin if no controller is available.
    FVector AnchorLoc = FVector::ZeroVector;
    FVector AnchorForward = FVector::ForwardVector;
    if (APlayerController* ViewPC = UGameplayStatics::GetPlayerController(GetWorld(), 0))
    {
        FVector CamLoc; FRotator CamRot;
        ViewPC->GetPlayerViewPoint(CamLoc, CamRot);
        AnchorLoc = CamLoc;
        AnchorForward = CamRot.Vector().GetSafeNormal();
        if (AnchorForward.IsNearlyZero())
        {
            AnchorForward = FVector::ForwardVector;
        }
    }
    // Place the scaled scene in front of the camera (at eye level minus SandboxHeightOffset) so it's visible.
    SandboxTargetCenter = AnchorLoc + AnchorForward * SandboxForwardDistance + FVector(0, 0, SandboxHeightOffset);

    CurrentScale = InitialScale;
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: BeginPlay ChildActors=%d SceneCenter=%s AnchorLoc=%s TargetCenter=%s InitialScale=%f"),
        ChildActors.Num(), *SandboxSceneCenter.ToString(), *AnchorLoc.ToString(), *SandboxTargetCenter.ToString(), InitialScale);
    ApplySandboxScale();
    AdjustLightsForScale();
    UpdateStageVisibility();
    SetupPCDebugInput();

    // 按形状分类所有 ChildActors（用于精准动画）
    ClassifyActorsByShape();

    // 初始化泄漏 Actor（fire_cue + Plane + MoltenFlow + xielou_gaolu）
    InitLeakActors();

    // BeginPlay 时：所有 LeakActors 都隐藏（默认无泄露，按 1 才会出现）
    // 发光材质用 SetActorHiddenInGame（缩小到 0.01 依然发光）
    for (AActor* L : LeakActors)
    {
        if (!L) continue;
        L->SetActorHiddenInGame(true);
    }

    // ── UI 创建：双轨方案，不判断 HMD ──
    //   同时创建两种 UI，各自在适用环境渲染：
    //   1) 2D Viewport UI（AddToViewport）：普通 PIE 里可见，VR Preview / MR 上不渲染（无妨）
    //   2) World Space 面板（AEyeAnchoredUIPanel）：VR Preview / MR 眼镜里可见，普通 PIE 里不显示（无妨）
    //   这样不需要任何 HMD 判断，所有场景都有 UI。
    UWorld* W = GetWorld();

    // ── (1) 2D Viewport UI ──
    {
        APlayerController* LocalPC = W ? W->GetFirstPlayerController() : nullptr;
        if (W && LocalPC)
        {
            if (UMR3PanelWidget* ViewportPanel = CreateWidget<UMR3PanelWidget>(LocalPC, UMR3PanelWidget::StaticClass()))
            {
                ViewportPanel->AddToViewport(9999);
                ViewportPanel->SetMouseDebugMode(true);
                ViewportPanel->SetupSandboxRefs(this, nullptr);
                UE_LOG(LogTemp, Error, TEXT("=== VIEWPORT UI CREATED on PC=%s ==="), *LocalPC->GetName());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("=== VIEWPORT UI CREATE WIDGET FAILED ==="));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("=== VIEWPORT UI NO PC ==="));
        }
    }

    // ── (2) World Space 面板（VR Preview / MR 眼镜用） ──
    {
        FActorSpawnParameters Sp;
        Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AEyeAnchoredUIPanel* PanelActor = W ? W->SpawnActor<AEyeAnchoredUIPanel>(
            AEyeAnchoredUIPanel::StaticClass(), FTransform::Identity, Sp) : nullptr;
        if (PanelActor)
        {
            PanelActor->SetMouseDebugMode(false);

            UMRHandUIInteractor* Interactor = NewObject<UMRHandUIInteractor>(
                PanelActor, UMRHandUIInteractor::StaticClass());
            if (Interactor)
            {
                Interactor->RegisterComponent();
                Interactor->bMouseDebugMode = false;
                Interactor->bDebugDraw = false;
                RegisterHandUIInteractor(Interactor);
            }
            PanelActor->SetSandboxRefs(this, Interactor);

            UE_LOG(LogTemp, Error, TEXT("=== WORLD SPACE PANEL SPAWNED: panel=%s, interactor=%s ==="),
                *PanelActor->GetName(), Interactor ? *Interactor->GetName() : TEXT("NULL"));
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("=== WORLD SPACE PANEL SPAWN FAILED ==="));
        }
    }
}

void AMRSandboxRoot::InitLeakActors()
{
    LeakActors.Empty();
    LeakOriginalScales.Empty();
    for (AActor* A : ChildActors)
    {
        if (!A) continue;
        const FString LL = A->GetActorLabel().ToLower();
        // 只收集泄露元素（fire_cue + molten_metal_plane + molten_metal_flow）
        // 不收集 xielou_gaolu（它是高炉，不是泄露元素）
        if (LL.Contains(TEXT("fire_cue")) ||
            LL.Contains(TEXT("molten_metal_plane")) ||
            LL.Contains(TEXT("molten_metal_flow")))
        {
            LeakActors.Add(A);
            LeakOriginalScales.Add(A->GetActorScale3D());
            UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Leak actor '%s' originalScale=%s"), *LL, *A->GetActorScale3D().ToString());
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: LeakActors=%d (fire_cue/plane/flow)"), LeakActors.Num());
}

void AMRSandboxRoot::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateHandInteraction(DeltaTime);
    UpdateAnimation(DeltaTime);
    FeedHandUITransform();
}

void AMRSandboxRoot::ApplySandboxScale()
{
    // Scale each ChildActor in place. We do NOT use Attach because the level's static actors
    // (Brush / StaticMesh / NavMesh) cannot be attached to a Movable root, and our sandbox root
    // is Movable (driven by Tick gestures). Scaling in place preserves the original world layout.
    //
    // Position formula: NewLocation = SandboxTargetCenter + (OriginalLocation - SandboxSceneCenter) * CurrentScale
    // so the whole scaled scene sits in front of the player, not at the world origin.
    for (int32 i = 0; i < ChildActors.Num(); ++i)
    {
        AActor* Child = ChildActors[i];
        if (!Child || Child == this)
        {
            continue;
        }

        // Force every scene component on this child to Movable so SetActorLocation / SetActorScale3D
        // actually take effect (the level's brush / StaticMesh / nav components default to Static,
        // and Static components silently ignore SetActorLocation, leaving them at their original spots).
        TArray<USceneComponent*> Comps;
        Child->GetComponents<USceneComponent>(Comps);
        for (USceneComponent* C : Comps)
        {
            if (C && C->Mobility != EComponentMobility::Movable)
            {
                C->SetMobility(EComponentMobility::Movable);
            }
        }

        const FTransform& OT = OriginalTransforms.IsValidIndex(i) ? OriginalTransforms[i] : Child->GetActorTransform();

        // 相对场景中心 → 旋转(Yaw) → 缩放 → 平移 + 目标中心
        const FQuat SandboxQuat = FQuat(FVector::UpVector, FMath::DegreesToRadians(SandboxYaw));
        const FVector RelativePos = OT.GetLocation() - SandboxSceneCenter;
        const FVector RotatedPos = SandboxQuat.RotateVector(RelativePos);
        const FVector ScaledPos = RotatedPos * CurrentScale;
        const FVector NewLocation = SandboxTargetCenter + SandboxTranslation + ScaledPos;

        const FVector NewScale = OT.GetScale3D() * CurrentScale;
        const FQuat NewRotation = SandboxQuat * OT.GetRotation();

        // LeakActors 的缩放由动画管理（原始 × 动画因子 × CurrentScale）
        if (!LeakActors.Contains(Child))
        {
            Child->SetActorScale3D(NewScale);
        }
        Child->SetActorRotation(NewRotation);
        Child->SetActorLocation(NewLocation, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

void AMRSandboxRoot::ResetSandbox()
{
    SandboxYaw = 0.0f;
    SandboxTranslation = FVector::ZeroVector;
    CurrentScale = InitialScale;
    ApplySandboxScale();
}

void AMRSandboxRoot::SetStage(int32 Stage)
{
    CurrentStage = FMath::Clamp(Stage, 1, 4);
    UpdateStageVisibility();

    // 启动对应动画
    AnimationTime = 0.0f;
    switch (CurrentStage)
    {
        case 1: CurrentAnimation = ESandboxAnimation::Stage1_Leak; break;
        case 2: CurrentAnimation = ESandboxAnimation::Stage2_Scan; break;
        case 3: CurrentAnimation = ESandboxAnimation::Stage3_Extinguish; break;
        case 4: CurrentAnimation = ESandboxAnimation::Stage4_Medical; break;
        default: CurrentAnimation = ESandboxAnimation::None; break;
    }
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: SetStage(%d) Firetrucks=%d Capsules=%d Leak=%d UAV=%d Scale=%f"),
        CurrentStage, FiretruckActors.Num(), CapsuleActors.Num(), LeakActors.Num(), SphereActors.Num(), CurrentScale);
}

void AMRSandboxRoot::UpdateStageVisibility()
{
    auto SetVisibility = [this](TArray<AActor*>& Actors, bool bVisible)
    {
        for (AActor* A : Actors)
        {
            if (A)
            {
                A->SetActorHiddenInGame(!bVisible);
            }
        }
    };

    SetVisibility(Stage1Actors, CurrentStage == 1);
    SetVisibility(Stage2Actors, CurrentStage == 2);
    SetVisibility(Stage3Actors, CurrentStage == 3);
    SetVisibility(Stage4Actors, CurrentStage == 4);
}

void AMRSandboxRoot::CollectFactoryActorsFromLevel()
{
    ChildActors.Empty();

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const TArray<FString> ExcludedNames = {
        TEXT("SkyLight"), TEXT("SkyAtmosphere"), TEXT("DirectionalLight"),
        TEXT("PlayerStart"), TEXT("PostProcessVolume"), TEXT("VRSpectator"),
        TEXT("VRPawn"), TEXT("BP_MRControls"), TEXT("BP_TrackedHands"),
        TEXT("GameMode"), TEXT("DefaultPawn"), TEXT("Camera"),
        TEXT("VolumetricCloud"), TEXT("ExponentialHeightFog")
    };

    for (TActorIterator<AActor> It(World); It; ++It)
    {
        AActor* A = *It;
        if (!A || A == this)
        {
            continue;
        }

        if (A->IsA(AMRSandboxRoot::StaticClass()))
        {
            continue;
        }

        const FString Name = A->GetName();
        bool bExcluded = false;
        for (const FString& Excluded : ExcludedNames)
        {
            if (Name.Contains(Excluded))
            {
                bExcluded = true;
                break;
            }
        }
        if (bExcluded)
        {
            continue;
        }

        ChildActors.Add(A);
    }
}

void AMRSandboxRoot::AttachChildrenInEditor()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Editor preview: place in front of the player start / camera.
    FVector Location = GetActorLocation();
    FRotator Rotation = GetActorRotation();

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC && PC->GetPawn())
    {
        const FVector PawnLoc = PC->GetPawn()->GetActorLocation();
        const FVector Forward = PC->GetPawn()->GetActorForwardVector();
        const FVector Right = PC->GetPawn()->GetActorRightVector();

        Location = PawnLoc
            + Forward * InitialOffset.X
            + Right * InitialOffset.Y
            + FVector::UpVector * (70.0f + HoverHeight);

        Rotation.Yaw = PC->GetPawn()->GetActorRotation().Yaw;
    }

    SetActorLocation(Location);
    SetActorRotation(Rotation);
    SetActorScale3D(FVector(InitialScale));

    OriginalTransforms.Empty(ChildActors.Num());
    int32 ValidCount = 0;
    SandboxSceneCenter = FVector::ZeroVector;
    for (AActor* Child : ChildActors)
    {
        const FTransform T = Child ? Child->GetActorTransform() : FTransform::Identity;
        OriginalTransforms.Add(T);
        if (Child)
        {
            SandboxSceneCenter += T.GetLocation();
            ++ValidCount;
        }
    }
    if (ValidCount > 0)
    {
        SandboxSceneCenter /= ValidCount;
    }
    SandboxTargetCenter = Location;
    CurrentScale = InitialScale;
    ApplySandboxScale();
}

void AMRSandboxRoot::ClearChildActors()
{
    ChildActors.Empty();
}

bool AMRSandboxRoot::GetHandPinchAndGrab(EControllerHand Hand, FVector& OutPinchPoint, bool& bOutPinching, bool& bOutGrabbing)
{
    IHandTracker* HandTracker = GetActiveHandTracker();
    if (!HandTracker || !HandTracker->IsHandTrackingStateValid())
    {
        return false;
    }

    TArray<FVector> Positions;
    TArray<FQuat> Rotations;
    TArray<float> Radii;
    if (!HandTracker->GetAllKeypointStates(Hand, Positions, Rotations, Radii))
    {
        return false;
    }

    if (Positions.Num() <= MRSandboxJoints::LittleTip)
    {
        return false;
    }

    const FVector PalmPos = Positions[MRSandboxJoints::Palm];
    const FVector Thumb = Positions[MRSandboxJoints::ThumbTip];
    const FVector Index = Positions[MRSandboxJoints::IndexTip];
    const FVector Middle = Positions[MRSandboxJoints::MiddleTip];
    const FVector Ring = Positions[MRSandboxJoints::RingTip];
    const FVector Little = Positions[MRSandboxJoints::LittleTip];

    OutPinchPoint = (Thumb + Index) * 0.5f;

    // 先算 Grab（四指都贴掌心）
    const float IndexCurl = FVector::Dist(Index, PalmPos);
    const float MiddleCurl = FVector::Dist(Middle, PalmPos);
    const float RingCurl = FVector::Dist(Ring, PalmPos);
    const float LittleCurl = FVector::Dist(Little, PalmPos);

    // 检测追踪丢失（手指全 0 = 手部追踪丢了，常见于握拳时手指互相遮挡）
    // 这种情况返回 false，不判任何手势（避免误判 grab）
    if (IndexCurl < 1.0f && MiddleCurl < 1.0f && RingCurl < 1.0f && LittleCurl < 1.0f)
    {
        return false;
    }

    // 3/4 投票判定握拳 + 滞回（避免帧间抖动）
    // 触发 grab：3+ 手指 curl < 阈值，且拇指食指没在捏合（PinchDist > PinchThreshold*1.5）
    // 解除 grab：1 或更少手指 curl < 阈值（要求几乎全张开才解除）
    // 中间区域（2 个手指弯曲）：保持上一帧状态
    int32 CurlCount = 0;
    if (IndexCurl < GrabThreshold) ++CurlCount;
    if (MiddleCurl < GrabThreshold) ++CurlCount;
    if (RingCurl < GrabThreshold) ++CurlCount;
    if (LittleCurl < GrabThreshold) ++CurlCount;

    // 先算 PinchDist（拇指食指距离），用于排除捏合时的 grab 误判
    const float PinchDist = FVector::Dist(Thumb, Index);
    // 只有拇指食指非常近（< 1.5cm，真正在捏）才排除 grab
    // 握拳时拇指食指天然靠近（2-2.5cm），不应排除 grab
    const bool bNotPinching = (PinchDist > 1.5f);

    bool& bLastGrab = (Hand == EControllerHand::Left) ? bLastLeftGrab : bLastRightGrab;
    if (bLastGrab)
    {
        // 已在 grab 状态：只有几乎全张开（CurlCount <= 1）才解除
        bOutGrabbing = (CurlCount >= 2) && bNotPinching;
    }
    else
    {
        // 不在 grab 状态：要 3+ 手指弯曲才触发
        bOutGrabbing = (CurlCount >= 3) && bNotPinching;
    }
    bLastGrab = bOutGrabbing;

    // PinchDist 已在上面计算（用于 grab 排除捏合）
    bOutPinching = (PinchDist < PinchThreshold) && !bOutGrabbing;

    static int32 CurlLogCount = 0;
    if (++CurlLogCount % 60 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Hand=%d PinchDist=%.2f IdxCurl=%.2f MidCurl=%.2f RingCurl=%.2f LitCurl=%.2f grab=%d pinch=%d"),
            (int32)Hand, PinchDist, IndexCurl, MiddleCurl, RingCurl, LittleCurl, bOutGrabbing, bOutPinching);
    }

    return true;
}

bool AMRSandboxRoot::IsIndexExtended(EControllerHand Hand)
{
    IHandTracker* HandTracker = GetActiveHandTracker();
    if (!HandTracker || !HandTracker->IsHandTrackingStateValid())
    {
        return false;
    }

    TArray<FVector> Positions;
    TArray<FQuat> Rotations;
    TArray<float> Radii;
    if (!HandTracker->GetAllKeypointStates(Hand, Positions, Rotations, Radii))
    {
        return false;
    }

    if (Positions.Num() <= MRSandboxJoints::LittleTip)
    {
        return false;
    }

    const FVector PalmPos = Positions[MRSandboxJoints::Palm];
    const FVector Index = Positions[MRSandboxJoints::IndexTip];
    const FVector Middle = Positions[MRSandboxJoints::MiddleTip];
    const FVector Ring = Positions[MRSandboxJoints::RingTip];
    const FVector Little = Positions[MRSandboxJoints::LittleTip];

    const float IndexCurl = FVector::Dist(Index, PalmPos);
    const float MiddleCurl = FVector::Dist(Middle, PalmPos);
    const float RingCurl = FVector::Dist(Ring, PalmPos);
    const float LittleCurl = FVector::Dist(Little, PalmPos);

    // 食指伸直判定（宽松版）：
    // - 食指离掌心 > 8cm（伸直，正常手 11-12cm，握拳 5-6cm）
    // - 中指/无名指/小指 3 个里有 2 个弯曲（< GrabThreshold）= 其余三指握拳
    int32 OtherCurlCount = 0;
    if (MiddleCurl < GrabThreshold) ++OtherCurlCount;
    if (RingCurl < GrabThreshold) ++OtherCurlCount;
    if (LittleCurl < GrabThreshold) ++OtherCurlCount;

    return (IndexCurl > 8.0f) && (OtherCurlCount >= 2);
}

void AMRSandboxRoot::UpdateHandInteraction(float DeltaTime)
{
    (void)DeltaTime;
    FVector LeftPinchPoint = FVector::ZeroVector;
    FVector RightPinchPoint = FVector::ZeroVector;

    bool bLeftPinching = false;
    bool bRightPinching = false;
    bool bLeftGrabbing = false;
    bool bRightGrabbing = false;

    const bool bLeftValid = GetHandPinchAndGrab(EControllerHand::Left, LeftPinchPoint, bLeftPinching, bLeftGrabbing);
    const bool bRightValid = GetHandPinchAndGrab(EControllerHand::Right, RightPinchPoint, bRightPinching, bRightGrabbing);

    static int32 TickCount = 0;
    if (++TickCount % 60 == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Tick bL=%d bR=%d pinch(L=%d R=%d) grab(L=%d R=%d) PinchTh=%.1f GrabTh=%.1f"),
            bLeftValid, bRightValid, bLeftPinching, bRightPinching, bLeftGrabbing, bRightGrabbing, PinchThreshold, GrabThreshold);
    }

    if (!bLeftValid && !bRightValid)
    {
        bWasBothPinching = false;
        bWasGrabbing = false;
        PreviousGrabPoint = FVector::ZeroVector;
        AimedActor = nullptr;
        return;
    }

    // 简化判定：不管 pinch 还是 grab，只要手"握紧"就算 active
    // 双手 active → 缩放/旋转；单手 active → 平移
    const bool bLeftActive = bLeftPinching || bLeftGrabbing;
    const bool bRightActive = bRightPinching || bRightGrabbing;
    const bool bBothActive = bLeftActive && bRightActive;
    const bool bSingleActive = (bLeftActive != bRightActive); // 一只 active 一只不 active

    // 双手握紧：缩放 OR 旋转（互斥——这一帧是距离变化大还是角度变化大）
    if (bBothActive)
    {
        const FVector SpanVec = RightPinchPoint - LeftPinchPoint;
        const float Span = SpanVec.Size();
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(SpanVec.Y, SpanVec.X));

        if (bWasBothPinching)
        {
            const float DeltaSpan = Span - PreviousSpan;
            float DeltaYaw = Yaw - PreviousYaw;
            if (DeltaYaw > 180.0f) DeltaYaw -= 360.0f;
            if (DeltaYaw < -180.0f) DeltaYaw += 360.0f;

            const bool bWantScale = FMath::Abs(DeltaSpan) > 0.3f;
            const bool bWantRotate = FMath::Abs(DeltaYaw) > 1.0f;

            if (bWantScale && !bWantRotate)
            {
                const float ScaleDelta = DeltaSpan * ScaleSensitivity * 0.01f;
                CurrentScale = FMath::Clamp(CurrentScale + ScaleDelta, MinScale * InitialScale, MaxScale * InitialScale);
                ApplySandboxScale();
            }
            else if (bWantRotate && !bWantScale)
            {
                SandboxYaw += DeltaYaw * RotationSensitivity;
                ApplySandboxScale();
            }
        }

        PreviousSpan = Span;
        PreviousYaw = Yaw;
        bWasBothPinching = true;
    }
    else
    {
        bWasBothPinching = false;
    }

    // 单手握紧：平移
    if (bSingleActive)
    {
        const FVector GrabPoint = bLeftActive ? LeftPinchPoint : RightPinchPoint;

        if (!bWasGrabbing && GrabGraceFrames <= 0)
        {
            // 真正的新握拳（宽容期已过）：重置基准点
            PreviousGrabPoint = GrabPoint;
            UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Grab started at %s"), *GrabPoint.ToString());
        }
        else
        {
            // 持续握拳 或 宽容期内的恢复：用上次的基准点算 Delta，保持连续
            const FVector Delta = GrabPoint - PreviousGrabPoint;
            SandboxTranslation.X += Delta.X * TranslationSensitivity;
            SandboxTranslation.Y += Delta.Y * TranslationSensitivity;
            ApplySandboxScale();

            static int32 TransLogCount = 0;
            if (++TransLogCount % 30 == 0)
            {
                UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Translate Delta=(%.2f,%.2f,%.2f) SandboxTranslation=%s"),
                    Delta.X, Delta.Y, Delta.Z, *SandboxTranslation.ToString());
            }

            PreviousGrabPoint = GrabPoint;
        }

        bWasGrabbing = true;
        GrabGraceFrames = 10; // 重置宽容期
    }
    else
    {
        // grab 丢失：进入宽容期，不立即重置（避免帧间抖动导致 Delta 归零）
        if (GrabGraceFrames > 0)
        {
            --GrabGraceFrames;
        }
        else
        {
            bWasGrabbing = false;
            PreviousGrabPoint = FVector::ZeroVector;
        }
    }

    // Index raycast（捏合时不射射线，但握拳/食指指向时可以射——食指指向本身会触发 grab）
    AimedActor = nullptr;

    if (bRightValid && !bRightPinching && IsIndexExtended(EControllerHand::Right))
    {
        IHandTracker* HandTracker = GetActiveHandTracker();
        TArray<FVector> Positions;
        TArray<FQuat> Rotations;
        TArray<float> Radii;
        if (HandTracker && HandTracker->GetAllKeypointStates(EControllerHand::Right, Positions, Rotations, Radii)
            && Positions.Num() > MRSandboxJoints::IndexTip)
        {
            const FVector Start = Positions[MRSandboxJoints::IndexTip];
            const FVector Dir = (Positions[MRSandboxJoints::IndexTip] - Positions[MRSandboxJoints::Palm]).GetSafeNormal();
            const FVector End = Start + Dir * 1000.0f;
            LastIndexTipPos = Start;

            // 画食指射线（绿色），让用户看到射线
            DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, -1.0f, 0, 0.2f);

            FHitResult Hit;
            FCollisionQueryParams Params;
            Params.AddIgnoredActor(this);

            if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
            {
                AimedActor = Hit.GetActor();
                // 命中点画红球
                DrawDebugPoint(GetWorld(), Hit.Location, 8.0f, FColor::Red, false, -1.0f, 0);
            }
        }
    }
}

void AMRSandboxRoot::ScaleSandbox(float DeltaScale)
{
    const float OldScale = CurrentScale;
    CurrentScale = FMath::Clamp(CurrentScale + DeltaScale, MinScale * InitialScale, MaxScale * InitialScale);
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: ScaleSandbox delta=%f old=%f new=%f"), DeltaScale, OldScale, CurrentScale);
    ApplySandboxScale();
}

void AMRSandboxRoot::RotateSandbox(float DeltaYaw)
{
    SandboxYaw += DeltaYaw;
    ApplySandboxScale();
}

void AMRSandboxRoot::TranslateSandboxXY(const FVector2D& Delta)
{
    SandboxTranslation.X += Delta.X;
    SandboxTranslation.Y += Delta.Y;
    ApplySandboxScale();
}

void AMRSandboxRoot::SetupPCDebugInput()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC)
    {
        return;
    }

    EnableInput(PC);

    // 鼠标控制视角（PC 调试用）——不设 InputMode，让 PIE 默认控制生效
    PC->bShowMouseCursor = true;
    PC->bEnableClickEvents = true;
    PC->bEnableMouseOverEvents = true;

    if (!InputComponent)
    {
        return;
    }

    // 滚轮缩放
    InputComponent->BindKey(EKeys::MouseScrollUp, IE_Pressed, this, &AMRSandboxRoot::OnScrollUp);
    InputComponent->BindKey(EKeys::MouseScrollDown, IE_Pressed, this, &AMRSandboxRoot::OnScrollDown);

    // 鼠标右键拖动 = 旋转视角（按住右键移动鼠标）
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AMRSandboxRoot::OnRMBDown);
    InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AMRSandboxRoot::OnRMBUp);

    // 键盘 1-4 触发阶段
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AMRSandboxRoot::OnKey1);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AMRSandboxRoot::OnKey2);
    InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &AMRSandboxRoot::OnKey3);
    InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &AMRSandboxRoot::OnKey4);
    InputComponent->BindKey(EKeys::R, IE_Pressed, this, &AMRSandboxRoot::OnKeyR);
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AMRSandboxRoot::OnKeyQ);
    InputComponent->BindKey(EKeys::E, IE_Pressed, this, &AMRSandboxRoot::OnKeyE);
}

void AMRSandboxRoot::OnScrollUp() { ScaleSandbox(ScrollScaleStep); }
void AMRSandboxRoot::OnScrollDown() { ScaleSandbox(-ScrollScaleStep); }
void AMRSandboxRoot::OnRMBDown() { bRMBDown = true; }
void AMRSandboxRoot::OnRMBUp() { bRMBDown = false; }
void AMRSandboxRoot::OnKey1() { SetStage(1); }
void AMRSandboxRoot::OnKey2() { SetStage(2); }
void AMRSandboxRoot::OnKey3() { SetStage(3); }
void AMRSandboxRoot::OnKey4() { SetStage(4); }
void AMRSandboxRoot::OnKeyR() { ResetSandbox(); }
void AMRSandboxRoot::OnKeyQ() { RotateSandbox(-KeyRotateStep); }
void AMRSandboxRoot::OnKeyE() { RotateSandbox(KeyRotateStep); }

void AMRSandboxRoot::AdjustLightsForScale()
{
    const float ScaleMul = CurrentScale;

    if (!bLightsCached)
    {
        CachedLights.Empty();
        for (AActor* Child : ChildActors)
        {
            if (!Child)
            {
                continue;
            }

            TArray<ULightComponent*> Lights;
            Child->GetComponents<ULightComponent>(Lights, false);
            for (ULightComponent* Light : Lights)
            {
                UPointLightComponent* PointLight = Cast<UPointLightComponent>(Light);
                if (!PointLight)
                {
                    continue;
                }

                FLightCache Cache;
                Cache.Comp = PointLight;
                Cache.OriginalRadius = PointLight->AttenuationRadius;
                CachedLights.Add(Cache);

                PointLight->SetAttenuationRadius(PointLight->AttenuationRadius * CurrentScale);
            }
        }
        bLightsCached = true;
        return;
    }

    for (const FLightCache& Cache : CachedLights)
    {
        if (Cache.Comp.IsValid())
        {
            UPointLightComponent* PointLight = Cast<UPointLightComponent>(Cache.Comp.Get());
            if (PointLight)
            {
                PointLight->SetAttenuationRadius(Cache.OriginalRadius * CurrentScale);
            }
        }
    }
}

void AMRSandboxRoot::RestoreLightRadii()
{
    for (const FLightCache& Cache : CachedLights)
    {
        if (Cache.Comp.IsValid())
        {
            UPointLightComponent* PointLight = Cast<UPointLightComponent>(Cache.Comp.Get());
            if (PointLight)
            {
                PointLight->SetAttenuationRadius(Cache.OriginalRadius);
            }
        }
    }

    bLightsCached = false;
    CachedLights.Empty();
}

void AMRSandboxRoot::OnPostWorldInit(UWorld* World, const UWorld::InitializationValues IVS)
{
    if (!World)
    {
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: OnPostWorldInit WorldType=%d"), (int32)World->WorldType);

    if (World->WorldType != EWorldType::Game && World->WorldType != EWorldType::PIE)
    {
        return;
    }

    TArray<AActor*> Existing;
    UGameplayStatics::GetAllActorsOfClass(World, AMRSandboxRoot::StaticClass(), Existing);
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: existing instances=%d"), Existing.Num());
    if (Existing.Num() > 0)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Spawned = World->SpawnActor<AMRSandboxRoot>(AMRSandboxRoot::StaticClass(), FTransform::Identity, Params);
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: auto-spawned valid=%d"), IsValid(Spawned));
}

void AMRSandboxRoot::SetAnimationPaused(bool bPaused)
{
    bAnimationPaused = bPaused;
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: animation %s"), bPaused ? TEXT("PAUSED") : TEXT("RUNNING"));
}

void AMRSandboxRoot::ToggleLeakVisibility()
{
    bLeakVisible = !bLeakVisible;
    for (int32 i = 0; i < LeakActors.Num(); ++i)
    {
        AActor* L = LeakActors[i];
        if (!L) continue;
        const FVector OrigScale = LeakOriginalScales.IsValidIndex(i) ? LeakOriginalScales[i] : FVector(1.0f);
        const FString LL = L->GetActorLabel().ToLower();
        if (LL.Contains(TEXT("fire_cue")))
        {
            L->SetActorHiddenInGame(!bLeakVisible);
        }
        else
        {
            // plane / flow：显示时恢复到正常范围（原始 × CurrentScale），隐藏时缩小到 0.01
            L->SetActorScale3D(bLeakVisible ? (OrigScale * CurrentScale) : FVector(0.01f));
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: leak visibility=%d"), bLeakVisible ? 1 : 0);
}

void AMRSandboxRoot::HighlightLeak()
{
    for (AActor* L : LeakActors)
    {
        if (!L) continue;
        SetActorColor(L, FLinearColor(1.0f, 0.5f, 0.0f, 1.0f)); // 橙色高亮
    }
    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: leak highlighted (%d actors)"), LeakActors.Num());
}

void AMRSandboxRoot::RegisterHandUIInteractor(UMRHandUIInteractor* InInteractor)
{
    HandUIInteractor = InInteractor;
    if (InInteractor)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: HandUIInteractor registered"));
    }
}

void AMRSandboxRoot::FeedHandUITransform()
{
    UMRHandUIInteractor* Interactor = HandUIInteractor.Get();
    if (!Interactor || !Interactor->Interaction)
    {
        return;
    }

    IHandTracker* HandTracker = GetActiveHandTracker();
    if (!HandTracker)
    {
        static bool bLoggedOnce = false;
        if (!bLoggedOnce)
        {
            bLoggedOnce = true;
            UE_LOG(LogTemp, Error, TEXT("=== FeedHandUITransform: NO HandTracker! interactor=%s ==="),
                *Interactor->GetName());
        }
        return;
    }

    TArray<FVector> Positions;
    TArray<FQuat>   Rotations;
    TArray<float>   Radii;
    if (!HandTracker->GetAllKeypointStates(EControllerHand::Right, Positions, Rotations, Radii)
        || Positions.Num() <= MRSandboxJoints::IndexTip)
    {
        static bool bLoggedNoData = false;
        if (!bLoggedNoData)
        {
            bLoggedNoData = true;
            UE_LOG(LogTemp, Error, TEXT("=== FeedHandUITransform: HandTracker OK but no right hand data ==="));
        }
        return;
    }

    const FVector TipLoc = Positions[MRSandboxJoints::IndexTip];

    // 射线方向：如果有 Palm 关节的旋转 + Z 轴指背 → 用 PalmZ，否则用 Tip-Palm 方向。
    FVector AimDir = FVector::ForwardVector;
    if (Positions.Num() > MRSandboxJoints::Palm)
    {
        const FVector Dir = (TipLoc - Positions[MRSandboxJoints::Palm]).GetSafeNormal();
        if (!Dir.IsNearlyZero())
        {
            AimDir = Dir;
        }
    }

    // 让 Interaction 的 X 轴正方向指向 AimDir。
    const FQuat AimQ = FQuat::FindBetweenNormals(FVector::ForwardVector, AimDir);
    const FTransform T(AimQ, TipLoc, FVector::OneVector);
    Interactor->SetHandTransform(T);

    // Pinch 检测：拇指和食指捏合（距离 < PinchThreshold）触发 UMG 点击
    if (Positions.Num() > MRSandboxJoints::ThumbTip)
    {
        const float PinchDist = FVector::Dist(TipLoc, Positions[MRSandboxJoints::ThumbTip]);
        Interactor->SetPinchPressed(PinchDist < PinchThreshold);
    }
}

void AMRSandboxRoot::ClassifyActorsByShape()
{
    CylinderActors.Empty();
    CapsuleActors.Empty();
    FactoryActors.Empty();
    FiretruckActors.Empty();
    SphereActors.Empty();
    PlaneActors.Empty();
    MoltenFlowActors.Empty();

    for (AActor* A : ChildActors)
    {
        if (!A) continue;

        FString Label = A->GetActorLabel().ToLower();
        if (Label.IsEmpty()) Label = A->GetName().ToLower();
        FString MeshName;
        TArray<UStaticMeshComponent*> MeshComps;
        A->GetComponents<UStaticMeshComponent>(MeshComps, false);
        for (UStaticMeshComponent* MC : MeshComps)
        {
            if (MC && MC->GetStaticMesh()) { MeshName = MC->GetStaticMesh()->GetName().ToLower(); break; }
        }

        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Actor '%s' mesh='%s'"), *Label, *MeshName);

        // 1. 熔融金属流（molten_metal_flow，cylinder mesh 但不是高炉）
        if (Label.Contains(TEXT("molten_metal_flow")) || Label.Contains(TEXT("metal_flow")))
        {
            MoltenFlowActors.Add(A); continue;
        }
        // 2. 泄漏金属平面（不含 gaolu——gaolu 归到高炉类）
        if ((Label.Contains(TEXT("molten_metal_plane")) ||
             Label.Contains(TEXT("xielou")) ||
             Label.Contains(TEXT("leak")) ||
             (Label.Contains(TEXT("plane")) && !Label.Contains(TEXT("floor"))))
           && !Label.Contains(TEXT("gaolu")))
        {
            PlaneActors.Add(A); continue;
        }
        // 3. 高炉（gaolu / luzi / xielou_gaolu，含 gaolu 就算高炉）
        if (Label.Contains(TEXT("gaolu")) || Label.Contains(TEXT("luzi")))
        {
            if (!Label.Contains(TEXT("dizuo")) && !Label.Contains(TEXT("molten_metal")))
            {
                CylinderActors.Add(A); continue;
            }
        }
        // 4. 人物胶囊（capsule / renwu / worker，不含 rescue）
        if ((Label.Contains(TEXT("capsule")) || Label.Contains(TEXT("renwu")) || Label.Contains(TEXT("worker")) ||
             Label.Contains(TEXT("yisheng")) || Label.Contains(TEXT("doctor")) ||
             Label.Contains(TEXT("shangyuan")) || Label.Contains(TEXT("patient")))
           && !Label.Contains(TEXT("rescue")))
        {
            CapsuleActors.Add(A); continue;
        }
        // 5. 厂房/急救站（changfang / rescue / factory）
        if (Label.Contains(TEXT("changfang")) || Label.Contains(TEXT("factory")) || Label.Contains(TEXT("rescue")))
        {
            FactoryActors.Add(A); continue;
        }
        // 6. 消防车（xiaofang）
        if (Label.Contains(TEXT("xiaofang")) || Label.Contains(TEXT("firetruck")))
        {
            FiretruckActors.Add(A); continue;
        }
        // 7. 无人机（按 Label 关键词匹配，不依赖 mesh——支持 UAV1/2/3、SphereReflectionCapture 等）
        FString LowerLabel = Label;
        if ((LowerLabel.Contains(TEXT("uav")) ||  // UAV1, UAV2, UAV3
             LowerLabel == TEXT("uav") ||
             LowerLabel.StartsWith(TEXT("uav")) ||
             LowerLabel.Contains(TEXT("sm_ball")) ||
             LowerLabel.Contains(TEXT("drone")) ||
             LowerLabel.Contains(TEXT("ball")))
           && !LowerLabel.Contains(TEXT("sky")))
        {
            SphereActors.Add(A); continue;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Classified - Gaolu=%d Renwu=%d Changfang=%d Xiaofang=%d UAV=%d Xielou=%d MoltenFlow=%d"),
        CylinderActors.Num(), CapsuleActors.Num(), FactoryActors.Num(),
        FiretruckActors.Num(), SphereActors.Num(), PlaneActors.Num(), MoltenFlowActors.Num());

    // 重排高炉：xielou_gaolu（泄露的高炉）排第一，其次 gaolu2
    for (int32 i = 0; i < CylinderActors.Num(); ++i)
    {
        if (CylinderActors[i] && CylinderActors[i]->GetActorLabel().ToLower().Contains(TEXT("xielou")))
        {
            if (i > 0) CylinderActors.Swap(0, i);
            break;
        }
    }
    // 没找到 xielou_gaolu，找 gaolu2
    if (CylinderActors.Num() > 0 && !CylinderActors[0]->GetActorLabel().ToLower().Contains(TEXT("xielou")))
    {
        for (int32 i = 0; i < CylinderActors.Num(); ++i)
        {
            if (CylinderActors[i] && CylinderActors[i]->GetActorLabel().ToLower().Contains(TEXT("gaolu")))
            {
                if (i > 0) CylinderActors.Swap(0, i);
                break;
            }
        }
    }
    if (CylinderActors.Num() > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Main furnace = '%s'"), *CylinderActors[0]->GetActorLabel());
    }
}

void AMRSandboxRoot::UpdateAnimation(float DeltaTime)
{
    if (CurrentAnimation == ESandboxAnimation::None)
    {
        return;
    }

    // 暂停：不累加时间、不移动 Actor，但**仍然执行 switch 里的绘制逻辑**
    // （DrawDebugSphere/DrawDebugLine/DrawDebugPoint/粒子 Spawn）。
    // 这样暂停时动画对象（无人机球、水柱、水雾、医生/伤员光晕）仍然可见，只是冻结在原地。
    if (bAnimationPaused)
    {
        DeltaTime = 0.0f;
    }

    AnimationTime += DeltaTime;

    switch (CurrentAnimation)
    {
        case ESandboxAnimation::Stage1_Leak:
        {
            // 阶段一：泄漏与避险
            FVector FurnaceLoc = SandboxSceneCenter;
            if (CylinderActors.Num() > 0 && CylinderActors[0])
            {
                FurnaceLoc = CylinderActors[0]->GetActorLocation();
            }

            // 泄漏元素：显示并放大（原始 × 动画因子 × CurrentScale，跟随沙盘缩放）
            for (int32 li = 0; li < LeakActors.Num(); ++li)
            {
                AActor* L = LeakActors[li];
                if (!L) continue;
                const FVector OrigScale = LeakOriginalScales.IsValidIndex(li) ? LeakOriginalScales[li] : FVector(1.0f);
                const FString LL = L->GetActorLabel().ToLower();
                // 所有 LeakActors 都显示
                L->SetActorHiddenInGame(false);
                if (LL.Contains(TEXT("fire_cue")))
                {
                    const FVector TargetScale = OrigScale * 2.0f * CurrentScale;
                    const FVector ActorScale = L->GetActorScale3D();
                    const FVector NewScale = FMath::VInterpTo(ActorScale, TargetScale, DeltaTime, 2.0f);
                    L->SetActorScale3D(NewScale);
                }
                else if (LL.Contains(TEXT("molten_metal_plane")) || LL.Contains(TEXT("molten_metal_flow")))
                {
                    const FVector TargetScale = OrigScale * 1.0f * CurrentScale;
                    const FVector ActorScale = L->GetActorScale3D();
                    const FVector NewScale = FMath::VInterpTo(ActorScale, TargetScale, DeltaTime, 2.0f);
                    L->SetActorScale3D(NewScale);
                }
            }

            // 人物胶囊远离高炉（慢速撤离，不飞出去）
            for (AActor* C : CapsuleActors)
            {
                if (!C) continue;
                const FVector Current = C->GetActorLocation();
                const FVector Away = (Current - FurnaceLoc).GetSafeNormal();
                // 只在水平面撤离（Z 不变），距离短一点，速度慢一点
                const FVector AwayFlat(Away.X, Away.Y, 0.0f);
                if (AwayFlat.IsNearlyZero()) continue;
                const FVector AwayNorm = AwayFlat.GetSafeNormal();
                const FVector Target = Current + AwayNorm * 100.0f * CurrentScale;
                const FVector New = FMath::VInterpTo(Current, Target, DeltaTime, 5.0f); // 速度 5（慢）
                C->SetActorLocation(New);
            }
            break;
        }
        case ESandboxAnimation::Stage2_Scan:
        {
            // 阶段二：侦察与指挥
            // - 无人机（球）围绕高炉做圆周扫描
            // - 高炉位置作为圆心
            FVector FurnaceLoc = SandboxSceneCenter;
            if (CylinderActors.Num() > 0 && CylinderActors[0])
            {
                FurnaceLoc = CylinderActors[0]->GetActorLocation();
            }

            const float Radius = 150.0f * CurrentScale;
            const float AngularSpeed = 1.0f;
            const float DroneHoverHeight = 300.0f * CurrentScale; // 高一点

            for (int32 i = 0; i < SphereActors.Num(); ++i)
            {
                AActor* S = SphereActors[i];
                if (!S) continue;
                const float Phase = i * 1.5f;
                const float Angle = AnimationTime * AngularSpeed + Phase;
                const FVector Target(
                    FurnaceLoc.X + FMath::Cos(Angle) * Radius,
                    FurnaceLoc.Y + FMath::Sin(Angle) * Radius,
                    FurnaceLoc.Z + DroneHoverHeight
                );
                const FVector New = FMath::VInterpTo(S->GetActorLocation(), Target, DeltaTime, 80.0f);
                S->SetActorLocation(New);
                // 给每个无人机画可见的球（UAV1/2/3 没 mesh，需要画球）
                DrawDebugSphere(GetWorld(), New, 10.0f * CurrentScale, 12, FColor::White, false, -1.0f, 0, 1.0f);
            }
            break;
        }
        case ESandboxAnimation::Stage3_Extinguish:
        {
            // 阶段三：封控与处置
            // - 消防车（xiaofang）向高炉推进，停在 80cm 处
            // - 消防车到位后喷水柱（蓝色射线从车到高炉）
            // - 厂房（changfang）闪烁高亮
            // - 熔融金属平面 + 流暗淡下来（缩放 1.0 → 0.4）
            FVector FurnaceLoc = SandboxSceneCenter;
            if (CylinderActors.Num() > 0 && CylinderActors[0])
            {
                FurnaceLoc = CylinderActors[0]->GetActorLocation();
            }

            for (AActor* B : FiretruckActors)
            {
                if (!B) continue;
                const FVector Current = B->GetActorLocation();
                const FVector ToFurnace = (FurnaceLoc - Current).GetSafeNormal();
                const FVector Target = FurnaceLoc - ToFurnace * 80.0f * CurrentScale;
                // 消防车 Z 保持初始高度（不悬浮）
                FVector NewLoc = FMath::VInterpTo(Current, Target, DeltaTime, 70.0f);
                NewLoc.Z = Current.Z; // Z 不变，只水平移动
                B->SetActorLocation(NewLoc);

                const float Dist = FVector::Dist(NewLoc, FurnaceLoc);
                if (Dist < 120.0f * CurrentScale)
                {
                    // 喷水柱（蓝色细线）
                    const FVector WaterStart = NewLoc + FVector(0, 0, 30.0f * CurrentScale);
                    const FVector WaterEnd = FurnaceLoc + FVector(0, 0, 20.0f * CurrentScale);
                    DrawDebugLine(GetWorld(), WaterStart, WaterEnd, FColor::Cyan, false, -1.0f, 0, 1.0f);

                    // 水雾粒子效果（每 0.2 秒 Spawn 一次 P_Steam_Lit）
                    if (!SteamPS)
                    {
                        SteamPS = LoadObject<UParticleSystem>(nullptr, TEXT("/Game/StarterContent/Particles/P_Steam_Lit.P_Steam_Lit"));
                        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Stage3 SteamPS loaded=%s"), SteamPS ? TEXT("YES") : TEXT("NO"));
                    }
                    if (SteamPS && AnimationTime - LastSteamTime > 0.2f)
                    {
                        LastSteamTime = AnimationTime;
                        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), SteamPS, FTransform(WaterEnd), true);
                        UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Stage3 steam spawned at %s"), *WaterEnd.ToString());
                    }
                }
            }

            // 厂房用颜色闪烁（不用缩放）
            const bool bFlash = FMath::Fmod(AnimationTime, 1.0f) < 0.5f;
            for (AActor* F : FactoryActors)
            {
                if (!F) continue;
                SetActorColor(F, bFlash ? FLinearColor(1.0f, 0.8f, 0.0f) : FLinearColor::White);
            }

            // 熔融金属暗淡下来（原始 × 动画因子 × CurrentScale）
            const float DimFactor = FMath::Clamp(1.0f - AnimationTime * 0.3f, 0.4f, 1.0f);
            const float FireFactor = FMath::Clamp(2.0f - AnimationTime * 0.5f, 1.0f, 2.0f);
            for (int32 li = 0; li < LeakActors.Num(); ++li)
            {
                AActor* L = LeakActors[li];
                if (!L) continue;
                const FVector OrigScale = LeakOriginalScales.IsValidIndex(li) ? LeakOriginalScales[li] : FVector(1.0f);
                const FString LL = L->GetActorLabel().ToLower();
                if (LL.Contains(TEXT("fire_cue")))
                {
                    L->SetActorScale3D(OrigScale * FireFactor * CurrentScale);
                }
                else if (LL.Contains(TEXT("molten_metal_plane")) || LL.Contains(TEXT("molten_metal_flow")))
                {
                    const FVector TargetScale = OrigScale * DimFactor * CurrentScale;
                    const FVector ActorScale = L->GetActorScale3D();
                    const FVector NewScale = FMath::VInterpTo(ActorScale, TargetScale, DeltaTime, 1.0f);
                    L->SetActorScale3D(NewScale);
                }
            }
            break;
        }
        case ESandboxAnimation::Stage4_Medical:
        {
            // 阶段四：医疗
            // - 急救站绿色光晕 + 颜色
            // - 伤员红色光晕 + 颜色
            // - 医生靠近伤员
            // - 熔融金属基本缓解（缩放 0.2 * CurrentScale）

            // 急救站用绿色光晕（DrawDebugPoint 比 MID 更可靠）
            for (AActor* F : FactoryActors)
            {
                if (!F) continue;
                FString FLow = F->GetActorLabel().ToLower();
                if (FLow.Contains(TEXT("rescue")) || FLow.Contains(TEXT("jijiu")))
                {
                    DrawDebugPoint(GetWorld(), F->GetActorLocation(), 20.0f * CurrentScale, FColor::Green, false, -1.0f, 0);
                    SetActorColor(F, FLinearColor::Green);
                    UE_LOG(LogTemp, Warning, TEXT("MRSandboxRoot: Stage4 rescue found at %s"), *F->GetActorLocation().ToString());
                }
            }

            // 熔融金属完全缓解（原始 × 动画因子 × CurrentScale）
            for (int32 li = 0; li < LeakActors.Num(); ++li)
            {
                AActor* L = LeakActors[li];
                if (!L) continue;
                const FVector OrigScale = LeakOriginalScales.IsValidIndex(li) ? LeakOriginalScales[li] : FVector(1.0f);
                const FString LL = L->GetActorLabel().ToLower();
                if (LL.Contains(TEXT("fire_cue")))
                {
                    const FVector TargetScale = OrigScale * 0.3f * CurrentScale;
                    const FVector ActorScale = L->GetActorScale3D();
                    const FVector NewScale = FMath::VInterpTo(ActorScale, TargetScale, DeltaTime, 1.0f);
                    L->SetActorScale3D(NewScale);
                }
                else if (LL.Contains(TEXT("molten_metal_plane")) || LL.Contains(TEXT("molten_metal_flow")))
                {
                    const FVector TargetScale = OrigScale * 0.2f * CurrentScale;
                    const FVector ActorScale = L->GetActorScale3D();
                    const FVector NewScale = FMath::VInterpTo(ActorScale, TargetScale, DeltaTime, 1.0f);
                    L->SetActorScale3D(NewScale);
                }
            }

            if (CapsuleActors.Num() < 2) break;

            // 找急救站（rescue）位置
            FVector RescueLoc = SandboxSceneCenter;
            for (AActor* F : FactoryActors)
            {
                if (!F) continue;
                const FString FLow = F->GetActorLabel().ToLower();
                if (FLow.Contains(TEXT("rescue")) || FLow.Contains(TEXT("jijiu")))
                {
                    RescueLoc = F->GetActorLocation();
                    break;
                }
            }

            // 医生 = 离急救站最近的胶囊
            AActor* Doctor = nullptr;
            float BestDist = FLT_MAX;
            for (AActor* C : CapsuleActors)
            {
                if (!C) continue;
                const float D = FVector::Dist(C->GetActorLocation(), RescueLoc);
                if (D < BestDist) { BestDist = D; Doctor = C; }
            }
            if (!Doctor) break;

            // 伤员 = 其他胶囊，变红闪烁
            for (AActor* C : CapsuleActors)
            {
                if (!C || C == Doctor) continue;

                const bool bPatientFlash = FMath::Fmod(AnimationTime, 0.5f) < 0.25f;
                if (bPatientFlash)
                {
                    DrawDebugPoint(GetWorld(), C->GetActorLocation(), 30.0f * CurrentScale, FColor::Red, false, -1.0f, 0);
                }
                SetActorColor(C, bPatientFlash ? FLinearColor::Red : FLinearColor::White);
            }

            // 医生向最近的伤员靠近
            AActor* NearestPatient = nullptr;
            float NearestDist = FLT_MAX;
            for (AActor* C : CapsuleActors)
            {
                if (!C || C == Doctor) continue;
                const float D = FVector::Dist(C->GetActorLocation(), Doctor->GetActorLocation());
                if (D < NearestDist) { NearestDist = D; NearestPatient = C; }
            }
            if (NearestPatient)
            {
                const FVector PatientLoc = NearestPatient->GetActorLocation();
                const FVector ToPatient = (PatientLoc - Doctor->GetActorLocation()).GetSafeNormal();
                if (!ToPatient.IsNearlyZero())
                {
                    const FVector Target = PatientLoc - ToPatient * 30.0f * CurrentScale;
                    const FVector New = FMath::VInterpTo(Doctor->GetActorLocation(), Target, DeltaTime, 90.0f);
                    Doctor->SetActorLocation(New);
                }
            }
            break;
        }
        default: break;
    }
}

void AMRSandboxRoot::SetActorColor(AActor* A, const FLinearColor& Color)
{
    if (!A) return;
    TArray<UStaticMeshComponent*> MeshComps;
    A->GetComponents<UStaticMeshComponent>(MeshComps, false);
    for (UStaticMeshComponent* MC : MeshComps)
    {
        if (!MC) continue;
        for (int32 i = 0; i < MC->GetNumMaterials(); ++i)
        {
            UMaterialInterface* Mat = MC->GetMaterial(i);
            if (!Mat) continue;
            UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat);
            if (!MID)
            {
                MID = UMaterialInstanceDynamic::Create(Mat, this);
                MC->SetMaterial(i, MID);
            }
            // 尝试多种常见参数名（不同材质用不同名字）
            MID->SetVectorParameterValue(TEXT("Color"), Color);
            MID->SetVectorParameterValue(TEXT("BaseColor"), Color);
            MID->SetVectorParameterValue(TEXT("DiffuseColor"), Color);
            MID->SetVectorParameterValue(TEXT("Diffuse"), Color);
            MID->SetVectorParameterValue(TEXT("Albedo"), Color);
            MID->SetVectorParameterValue(TEXT("Tint"), Color);
            MID->SetVectorParameterValue(TEXT("EmissiveColor"), Color * 2.0f);
            MID->SetVectorParameterValue(TEXT("Emissive"), Color * 2.0f);
        }
    }
}
