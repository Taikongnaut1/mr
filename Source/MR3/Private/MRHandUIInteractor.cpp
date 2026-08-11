// Copyright (c) Yuquan Sun. All rights reserved.

#include "MRHandUIInteractor.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/SceneComponent.h"
#include "InputCoreTypes.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

UMRHandUIInteractor::UMRHandUIInteractor()
{
    PrimaryComponentTick.bCanEverTick = true;

    Interaction = CreateDefaultSubobject<UWidgetInteractionComponent>(TEXT("Interaction"));
    Interaction->InteractionDistance = InteractionDistance;
    Interaction->InteractionSource   = EWidgetInteractionSource::World;
    Interaction->bShowDebug          = false;
    Interaction->DebugColor          = FLinearColor(0.f, 1.f, 0.f, 1.f);
    Interaction->DebugLineThickness  = 1.0f;
}

void UMRHandUIInteractor::InitializeWithHandSceneComponents(USceneComponent* InHandRoot)
{
    HandTrackingRoot = InHandRoot;
}

void UMRHandUIInteractor::SetHandTransform(const FTransform& NewTransform)
{
    if (Interaction)
    {
        Interaction->SetWorldTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }
}

void UMRHandUIInteractor::SetPinchPressed(bool bPressed)
{
    if (bLastPressed == bPressed || !Interaction) return;
    bLastPressed = bPressed;

    if (bPressed)
        Interaction->PressPointerKey(EKeys::LeftMouseButton);
    else
        Interaction->ReleasePointerKey(EKeys::LeftMouseButton);
}

void UMRHandUIInteractor::BeginPlay()
{
    Super::BeginPlay();
    if (Interaction)
    {
        Interaction->InteractionDistance = InteractionDistance;
        Interaction->bShowDebug          = false;
    }
}

void UMRHandUIInteractor::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);

    if (!Interaction) return;

    // 鼠标调试模式优先（PIE 中用鼠标替代手指）
    if (bMouseDebugMode)
    {
        ApplyMouseDebugRay();
    }
    else if (bAutoTrackHandRoot && HandTrackingRoot != nullptr)
    {
        const FTransform T = HandTrackingRoot->GetComponentTransform();
        Interaction->SetWorldTransform(T, false, nullptr, ETeleportType::TeleportPhysics);
    }

    if (bDebugDraw)
    {
        const FTransform TipT = Interaction->GetComponentTransform();
        const FVector Start = TipT.GetLocation();
        const FVector End   = Start + TipT.GetUnitAxis(EAxis::X) * Interaction->InteractionDistance;
        DrawDebugLine(GetWorld(), Start, End, FColor::Green, false, 0.f, 0, 1.f);
    }
}

void UMRHandUIInteractor::ApplyMouseDebugRay()
{
    if (!Interaction) return;

    UWorld* W = GetWorld();
    APlayerController* PC = W ? W->GetFirstPlayerController() : nullptr;
    if (!PC) return;

    float MX, MY;
    if (!PC->GetMousePosition(MX, MY)) return;

    FVector Origin, Dir;
    if (!PC->DeprojectScreenPositionToWorld(MX, MY, Origin, Dir)) return;

    const FQuat AimQ = FQuat::FindBetweenNormals(FVector::ForwardVector, Dir.GetSafeNormal());
    const FTransform T(AimQ, Origin, FVector::OneVector);
    Interaction->SetWorldTransform(T, false, nullptr, ETeleportType::TeleportPhysics);

    if (PC->IsInputKeyDown(EKeys::LeftMouseButton))
        SetPinchPressed(true);
    else
        SetPinchPressed(false);
}
