// Copyright (c) Yuquan Sun. All rights reserved.

#include "Modules/ModuleManager.h"
#include "MRSandboxRoot.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"

class FMR3Module : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UE_LOG(LogTemp, Warning, TEXT("MR3: FMR3Module::StartupModule called - registering auto-spawn delegate"));

        FWorldDelegates::OnPostWorldInitialization.AddLambda(
            [](UWorld* World, const UWorld::InitializationValues IVS)
            {
                AMRSandboxRoot::OnPostWorldInit(World, IVS);
            });

        // Force CDO construction after engine init (in case constructor-side code needs it).
        FCoreDelegates::OnPostEngineInit.AddLambda([]()
        {
            AMRSandboxRoot::StaticClass();
        });
    }

    virtual void ShutdownModule() override
    {
    }
};

IMPLEMENT_MODULE(FMR3Module, MR3)
