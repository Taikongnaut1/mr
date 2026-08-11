#include "Streaming/XvisioStream.h"
#include "UXVisioStreamWorldSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

static UXVisioStreamWorldSubsystem* GetStreamSubsystem(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	return World ? World->GetSubsystem<UXVisioStreamWorldSubsystem>() : nullptr;
}

int32 UXvisioStream::CreateClient(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->CreateClient();
	return -1;
}

int32 UXvisioStream::CreateClientWithIp(UObject* WorldContextObject, const FString& ip)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->CreateClientWithIp(ip);
	return -1;
}

void UXvisioStream::LoadXvisioDLL(UObject* WorldContextObject, const FString& ClientIP)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->LoadXvisioDLL(ClientIP);
}

void UXvisioStream::FreeXvisioDLL(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->FreeXvisioDLL();
}

void UXvisioStream::StartEyeTracking(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->StartEyeTracking();
}

void UXvisioStream::EndEyeTracking(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->EndEyeTracking();
}

bool UXvisioStream::GetEyeReceivedData(UObject* WorldContextObject, FEyeData& OutEyeData)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->GetEyeReceivedData(OutEyeData);
	return false;
}

void UXvisioStream::StartASR(UObject* WorldContextObject, const FString& bnf)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->StartASR(bnf);
}

FString UXvisioStream::GetReceivedMessage(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->GetReceivedMessage();
	return TEXT("");
}

void UXvisioStream::EndASR(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->EndASR();
}

void UXvisioStream::CloseClient(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->CloseClient();
}

void UXvisioStream::ParseAsrResultMessage(const FString& MsgJsonString)
{
	UXVisioStreamWorldSubsystem::ParseAsrResultMessage(MsgJsonString);
}

void UXvisioStream::StartCSLAMMap(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->StartCSLAMMap();
}

void UXvisioStream::SetMapPath(UObject* WorldContextObject, const FString& filePath)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->SetMapPath(filePath);
}

void UXvisioStream::SaveSlamMap(UObject* WorldContextObject, const FString& pcPath, const FString& fileName)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->SaveSlamMap(pcPath, fileName);
}

void UXvisioStream::SendMapFile(UObject* WorldContextObject, const FString& filePath)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->SendMapFile(filePath);
}

// ---- AprilTag ----

bool UXvisioStream::GetApriltag(UObject* WorldContextObject, int32& OutTagCount)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->GetApriltag(OutTagCount);
	OutTagCount = 0;
	return false;
}

bool UXvisioStream::GetApriltagData(UObject* WorldContextObject, int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		return Sub->GetApriltagData(Index, OutTagID, OutPosition, OutOrientation, OutQuaternion, OutConfidence);
	return false;
}

void UXvisioStream::StopApriltag(UObject* WorldContextObject)
{
	if (auto* Sub = GetStreamSubsystem(WorldContextObject))
		Sub->StopApriltag();
}