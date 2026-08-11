// Copyright (c) Yuquan Sun. All rights reserved.

#include "MREyeAnchoredUIPanel.h"
#include "MR3PanelWidget.h"
#include "MRHandUIInteractor.h"
#include "MRSandboxRoot.h"

#include "Components/WidgetComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AEyeAnchoredUIPanel::AEyeAnchoredUIPanel()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    WidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComp"));
    WidgetComp->SetupAttachment(Root);
    // 构造函数先按 default 设。BeginPlay 会根据 bUseScreenSpace 再决定。
    WidgetComp->SetWidgetSpace(EWidgetSpace::World);
    WidgetComp->SetDrawSize(WidgetDrawSize);
    WidgetComp->SetPivot(FVector2D(0.5f, 0.5f));
    WidgetComp->SetGenerateOverlapEvents(false);
    WidgetComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WidgetComp->SetTwoSided(true);
    WidgetComp->SetCastShadow(false);
    WidgetComp->SetReceivesDecals(false);

    // 关键：缩放。1920 cm 像素 → 60cm 物理宽度。
    const float UMGScale = 60.0f / WidgetDrawSize.X;
    WidgetComp->SetRelativeScale3D(FVector(UMGScale));

    WidgetClass = UMR3PanelWidget::StaticClass();
}

void AEyeAnchoredUIPanel::SetWidgetClass(TSubclassOf<UUserWidget> InClass)
{
    WidgetClass = InClass;
    if (WidgetComp)
    {
        WidgetComp->SetWidgetClass(InClass);
        WidgetComp->SetDrawSize(WidgetDrawSize);
    }
}

void AEyeAnchoredUIPanel::SetMouseDebugMode(bool bEnabled)
{
    bMouseDebugMode = bEnabled;
    if (WidgetComp)
    {
        if (UMR3PanelWidget* Panel = Cast<UMR3PanelWidget>(WidgetComp->GetUserWidgetObject()))
        {
            Panel->SetMouseDebugMode(bEnabled);
        }
    }
}

void AEyeAnchoredUIPanel::SetSandboxRefs(AMRSandboxRoot* InSandbox, UMRHandUIInteractor* InInteractor)
{
    if (WidgetComp)
    {
        if (UMR3PanelWidget* Panel = Cast<UMR3PanelWidget>(WidgetComp->GetUserWidgetObject()))
        {
            Panel->SetupSandboxRefs(InSandbox, InInteractor);
        }
    }
}

void AEyeAnchoredUIPanel::BeginPlay()
{
    Super::BeginPlay();

    if (WidgetComp && WidgetClass)
    {
        WidgetComp->SetWidgetClass(WidgetClass);
    }

    // ── 模式切换 ──
    if (WidgetComp)
    {
        if (bUseScreenSpace)
        {
            WidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
        }
        else
        {
            WidgetComp->SetWidgetSpace(EWidgetSpace::World);
            WidgetComp->SetPivot(FVector2D(0.5f, 0.5f));
            WidgetComp->SetDrawSize(WidgetDrawSize);
            WidgetComp->SetRelativeScale3D(FVector(60.0f / WidgetDrawSize.X));
        }
    }

    TryBindCamera();

    UE_LOG(LogTemp, Error, TEXT("=== EYEANCHOR BEGINPLAY: screen=%d wet=%s ==="),
        bUseScreenSpace ? 1 : 0, WidgetComp ? *WidgetComp->GetName() : TEXT("NULL"));

    // Screen 模式下也延迟一帧来确认 widget 创建好。
    GetWorld()->GetTimerManager().SetTimerForNextTick([this]()
    {
        if (!WidgetComp) return;
        if (UMR3PanelWidget* Panel = Cast<UMR3PanelWidget>(WidgetComp->GetUserWidgetObject()))
        {
            Panel->SetMouseDebugMode(bMouseDebugMode);
            UE_LOG(LogTemp, Warning, TEXT("AEyeAnchoredUIPanel: widget ready, screenSpace=%d, mouseDebug=%d"),
                bUseScreenSpace ? 1 : 0, bMouseDebugMode ? 1 : 0);
        }
    });
}

void AEyeAnchoredUIPanel::TryBindCamera()
{
    if (CachedCamera.IsValid()) return;

    if (OverrideCamera)
    {
        CachedCamera = OverrideCamera;
        return;
    }

    if (APawn* Pawn = UGameplayStatics::GetPlayerPawn(this, 0))
    {
        if (UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>())
        {
            CachedCamera = Cam;
            return;
        }
    }

    CachedCamera = nullptr;
}

void AEyeAnchoredUIPanel::Tick(float DeltaSeconds)
{
    // Screen 模式下不需要 Tick 跟随相机（已经钉在屏幕前面）。
    if (bUseScreenSpace) return;
    Super::Tick(DeltaSeconds);

    FVector  CamLoc = FVector::ZeroVector;
    FRotator CamRot = FRotator::ZeroRotator;
    bool bHaveCam = false;

    if (UCameraComponent* Cam = CachedCamera.Get())
    {
        CamLoc = Cam->GetComponentLocation();
        CamRot = Cam->GetComponentRotation();
        bHaveCam = true;
    }
    else if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (APlayerCameraManager* CM = PC->PlayerCameraManager)
        {
            CamLoc = CM->GetCameraLocation();
            CamRot = CM->GetCameraRotation();
            bHaveCam = true;
        }
    }

    if (!bHaveCam)
    {
        // Fallback：用玩家 Pawn 的位置 + 朝向作初始定位，保证面板在世界可见位置。
        if (APawn* P = UGameplayStatics::GetPlayerPawn(this, 0))
        {
            CamLoc = P->GetActorLocation();
            CamRot = P->GetActorRotation();
        }
        else
        {
            TryBindCamera();
            return;
        }
    }

    SetActorRotation(CamRot);

    const FVector Right = CamRot.RotateVector(FVector::RightVector);
    const FVector Up    = CamRot.RotateVector(FVector::UpVector);
    const FVector Fwd   = CamRot.RotateVector(FVector::ForwardVector);

    const FVector NewLoc = CamLoc + Fwd * Distance + Right * HorizontalOffset + Up * VerticalOffset;
    SetActorLocation(NewLoc, false, nullptr, ETeleportType::TeleportPhysics);
}
