// Copyright (c) Yuquan Sun. All rights reserved.

#include "MREyeAnchoredUIPanel.h"
#include "MR3PanelWidget.h"
#include "MRHandUIInteractor.h"
#include "MRSandboxRoot.h"

#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "TextureResource.h"
#include "Engine/Engine.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "IStereoLayers.h"
#include "StereoRendering.h"
#include "IHandTracker.h"
#include "Features/IModularFeatures.h"

namespace
{
    // Xvisio 手部追踪关节索引（与 MRSandboxJoints 一致）
    constexpr int32 JointPalm = 0;
    constexpr int32 JointIndexTip = 10;

    IHandTracker* GetActiveHandTracker()
    {
        if (!IModularFeatures::Get().IsModularFeatureAvailable(IHandTracker::GetModularFeatureName()))
        {
            return nullptr;
        }
        return &IModularFeatures::Get().GetModularFeature<IHandTracker>(IHandTracker::GetModularFeatureName());
    }
}

AEyeAnchoredUIPanel::AEyeAnchoredUIPanel()
{
    PrimaryActorTick.bCanEverTick = true;

    WidgetClass = UMR3PanelWidget::StaticClass();
}

void AEyeAnchoredUIPanel::SetWidgetClass(TSubclassOf<UUserWidget> InClass)
{
    WidgetClass = InClass;
}

void AEyeAnchoredUIPanel::SetMouseDebugMode(bool bEnabled)
{
    bMouseDebugMode = bEnabled;
    if (PanelWidget)
    {
        PanelWidget->SetMouseDebugMode(bEnabled);
    }
}

void AEyeAnchoredUIPanel::SetSandboxRefs(AMRSandboxRoot* InSandbox, UMRHandUIInteractor* InInteractor)
{
    PendingSandbox = InSandbox;
    PendingInteractor = InInteractor;
    bSandboxRefsApplied = false;

    if (PanelWidget)
    {
        PanelWidget->SetupSandboxRefs(InSandbox, InInteractor);
        bSandboxRefsApplied = true;
    }
}

void AEyeAnchoredUIPanel::BeginPlay()
{
    Super::BeginPlay();

    // ── 1. 创建 RenderTarget ──
    PanelRT = NewObject<UTextureRenderTarget2D>(this);
    PanelRT->InitAutoFormat((int32)WidgetDrawSize.X, (int32)WidgetDrawSize.Y);
    PanelRT->ClearColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.0f);
    PanelRT->UpdateResourceImmediate();

    // ── 2. 创建 UMG Widget ──
    PanelWidget = CreateWidget<UMR3PanelWidget>(GetWorld(), WidgetClass);
    if (PanelWidget)
    {
        PanelWidget->SetMouseDebugMode(bMouseDebugMode);
        if (PendingSandbox.IsValid())
        {
            PanelWidget->SetupSandboxRefs(PendingSandbox.Get(), PendingInteractor.Get());
            bSandboxRefsApplied = true;
        }
    }

    // ── 3. 创建 Slate 渲染器 ──
    WidgetRenderer = new FWidgetRenderer(false);

    // ── 4. 延迟一帧创建 FaceLocked Stereo Layer（确保 RenderTarget 资源已就绪） ──
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        CreateStereoLayer();
    });
}

void AEyeAnchoredUIPanel::CreateStereoLayer()
{
    if (!PanelRT || !GEngine || !GEngine->StereoRenderingDevice.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("AEyeAnchoredUIPanel: CreateStereoLayer skipped (no RT or XR device)"));
        return;
    }

    IStereoLayers* StereoLayers = GEngine->StereoRenderingDevice->GetStereoLayers();
    if (!StereoLayers)
    {
        UE_LOG(LogTemp, Warning, TEXT("AEyeAnchoredUIPanel: GetStereoLayers() returned null"));
        return;
    }

    // RenderTarget 的 RHI 纹理
    FTextureRenderTarget2DResource* RTRes =
        static_cast<FTextureRenderTarget2DResource*>(PanelRT->GetResource());
    if (!RTRes)
    {
        UE_LOG(LogTemp, Warning, TEXT("AEyeAnchoredUIPanel: RenderTarget resource not ready"));
        return;
    }
    FTextureRHIRef RTTexture = RTRes->GetRenderTargetTexture();

    IStereoLayers::FLayerDesc Desc;
    // FaceLocked：transform 是 view space（+X 前方，+Y 右，+Z 上），单位 cm
    Desc.Transform = FTransform(FVector(Distance, 0.f, VerticalOffset));
    Desc.QuadSize = QuadSize;
    Desc.PositionType = IStereoLayers::ELayerType::FaceLocked;
    Desc.Texture = RTTexture;
    Desc.Flags = IStereoLayers::ELayerFlags::LAYER_FLAG_TEX_CONTINUOUS_UPDATE;

    StereoLayerId = StereoLayers->CreateLayer(Desc);

    UE_LOG(LogTemp, Warning, TEXT("AEyeAnchoredUIPanel: FaceLocked layer created id=%u size=%s dist=%.0f"),
        StereoLayerId, *QuadSize.ToString(), Distance);
}

void AEyeAnchoredUIPanel::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // 每帧把 UMG 渲染到 RenderTarget（FaceLocked 层会连续采样这个纹理）
    if (WidgetRenderer && PanelWidget && PanelRT)
    {
        WidgetRenderer->DrawWidget(
            PanelRT,
            PanelWidget->TakeWidget(),
            WidgetDrawSize,
            DeltaSeconds);
    }

    // 通知 XR runtime 纹理已更新
    if (StereoLayerId != 0 && GEngine && GEngine->StereoRenderingDevice.IsValid())
    {
        if (IStereoLayers* StereoLayers = GEngine->StereoRenderingDevice->GetStereoLayers())
        {
            StereoLayers->MarkTextureForUpdate(StereoLayerId);
        }
    }

    // 手部射线交互（FaceLocked 层无碰撞体，手动做射线-面板平面求交）
    UpdateHandInteraction();
}

void AEyeAnchoredUIPanel::UpdateHandInteraction()
{
    if (!PanelWidget) return;

    bool bHitThisFrame = false;
    FVector2D HitScreenPos = FVector2D::ZeroVector;

    IHandTracker* HandTracker = GetActiveHandTracker();
    if (HandTracker)
    {
        TArray<FVector> Positions;
        TArray<FQuat> Rotations;
        TArray<float> Radii;
        if (HandTracker->GetAllKeypointStates(EControllerHand::Right, Positions, Rotations, Radii)
            && Positions.Num() > JointIndexTip)
        {
            const FVector TipLoc = Positions[JointIndexTip];
            const FVector PalmLoc = Positions[JointPalm];
            const FVector AimDir = (TipLoc - PalmLoc).GetSafeNormal();

            APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0);
            if (!AimDir.IsNearlyZero() && Pawn)
            {
                FVector EyeLoc;
                FRotator EyeRot;
                Pawn->GetActorEyesViewPoint(EyeLoc, EyeRot);

                const FVector Fwd = EyeRot.Vector();
                const FVector Right = FRotationMatrix(EyeRot).GetScaledAxis(EAxis::Y);
                const FVector Up = FRotationMatrix(EyeRot).GetScaledAxis(EAxis::Z);

                // 面板平面（与 FaceLocked 层的 transform 一致）
                const FVector PanelCenter = EyeLoc + Fwd * Distance + Up * VerticalOffset;
                const FVector PanelNormal = -Fwd;

                const float Denom = FVector::DotProduct(AimDir, PanelNormal);
                if (FMath::Abs(Denom) >= 1e-5f)
                {
                    const float t = FVector::DotProduct(PanelCenter - TipLoc, PanelNormal) / Denom;
                    if (t >= 0.f)
                    {
                        const FVector HitPoint = TipLoc + AimDir * t;
                        const FVector Offset = HitPoint - PanelCenter;
                        const float LocalX = FVector::DotProduct(Offset, Right);
                        const float LocalY = FVector::DotProduct(Offset, Up);

                        const float HalfW = QuadSize.X * 0.5f;
                        const float HalfH = QuadSize.Y * 0.5f;
                        if (FMath::Abs(LocalX) <= HalfW && FMath::Abs(LocalY) <= HalfH)
                        {
                            const float ScreenX = (LocalX / QuadSize.X + 0.5f) * WidgetDrawSize.X;
                            const float ScreenY = (0.5f - LocalY / QuadSize.Y) * WidgetDrawSize.Y;
                            HitScreenPos = FVector2D(ScreenX, ScreenY);
                            bHitThisFrame = true;
                        }
                    }
                }
            }
        }
    }

    // 悬停触发：射线停留在同一位置超过 HoverTriggerTime 秒 → 触发点击
    const float Now = GetWorld()->GetTimeSeconds();
    if (bHitThisFrame)
    {
        const bool bSameSpot = bHovering
            && FVector2D::DistSquared(HitScreenPos, LastHoverScreenPos) <= HoverMoveThreshold * HoverMoveThreshold;

        if (bSameSpot && (Now - HoverStartTime) >= HoverTriggerTime)
        {
            PanelWidget->HandleScreenTap(HitScreenPos);
            HoverStartTime = Now; // 重置计时，防止连续触发
            bHovering = false;    // 需移开再指向才能再次触发
        }
        else if (!bSameSpot)
        {
            LastHoverScreenPos = HitScreenPos;
            HoverStartTime = Now;
            bHovering = true;
        }
    }
    else
    {
        bHovering = false;
    }
}

void AEyeAnchoredUIPanel::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (StereoLayerId != 0 && GEngine && GEngine->StereoRenderingDevice.IsValid())
    {
        if (IStereoLayers* StereoLayers = GEngine->StereoRenderingDevice->GetStereoLayers())
        {
            StereoLayers->DestroyLayer(StereoLayerId);
        }
        StereoLayerId = 0;
    }

    if (WidgetRenderer)
    {
        BeginCleanup(WidgetRenderer);
        WidgetRenderer = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}
