// Copyright Epic Games, Inc. All Rights Reserved.

#include "Standalone/XvisioOpenXR.h"

#include "UXVisioXRWorldSubsystem.h"


namespace XvisioOpenXR
{

	
	FXVisioOpenXRModule* FXVisioOpenXRModule::Get()
	{
		return &FModuleManager::LoadModuleChecked<FXVisioOpenXRModule>("XvisioOpenXR");
	}

	bool FXVisioOpenXRModule::IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("XvisioOpenXR");
	}

	void FXVisioOpenXRModule::StartupModule()
	{
		UE_LOG(LogXvisioOpenXRPlugin, Log, TEXT("XVisioOpenXR StartupModule"));

		GazeCalibrationPlugin.Register();
		HandJointExtraPlugin.Register();
		KeyEventPlugin.Register();
		XvisioFunctionPlugin.Register();
	}

	void FXVisioOpenXRModule::ShutdownModule()
	{
		UE_LOG(LogXvisioOpenXRPlugin, Log, TEXT("XVisioOpenXR ShutdownModule"));

		GazeCalibrationPlugin.Unregister();
		HandJointExtraPlugin.Unregister();
		KeyEventPlugin.Unregister();
		XvisioFunctionPlugin.Unregister();
	}

} // namespace XvisioOpenXR


IMPLEMENT_MODULE(XvisioOpenXR::FXVisioOpenXRModule, XvisioOpenXR);


bool UXvisioOpenXR::StartEvent(UObject* WorldContextObject)
{
	if (!WorldContextObject)
		return false;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
		return false;
	

	UXVisioXRWorldSubsystem* Subsystem = World->GetSubsystem<UXVisioXRWorldSubsystem>();

	return Subsystem ? Subsystem->StartEvent() : false;
}

bool UXvisioOpenXR::StopEvent(UObject* WorldContextObject)
{

	if (!WorldContextObject)
		return false;

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
		return false;

	UXVisioXRWorldSubsystem* Subsystem = World->GetSubsystem<UXVisioXRWorldSubsystem>();

	return Subsystem ? Subsystem->StopEvent() : false;

}

bool UXvisioOpenXR::GetHandJointExtraData(EControllerHand Hand,
	TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations, TArray<float>& OutRadii)
{
	if (!XvisioOpenXR::FXVisioOpenXRModule::IsAvailable())
	{
		return false;
	}

	XvisioOpenXR::FXVisioOpenXRModule* Module = XvisioOpenXR::FXVisioOpenXRModule::Get();
	if (!Module)
	{
		return false;
	}

	return Module->GetHandJointExtraPlugin().GetHandJointExtraData(Hand,
		OutPositions, OutRotations, OutRadii);
}
