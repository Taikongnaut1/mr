#include "UXVisioStreamWorldSubsystem.h"

#include "Streaming/ASRDataRunNable.h"
#include "Streaming/EyeGazeDataRunnable.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "IXRTrackingSystem.h"
#include "Engine/Engine.h"

void UXVisioStreamWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Log, TEXT("UXVisioStreamWorldSubsystem Initialize"));

	// 获取 XR 追踪系统引用，用于 SLAM -> UE 坐标系转换
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == FName(TEXT("OpenXR"))))
	{
		XRTrackingSystem = GEngine->XRSystem.Get();
	}
}

void UXVisioStreamWorldSubsystem::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("UXVisioStreamWorldSubsystem Deinitialize"));
	StopApriltag();
	EndASR();
	EndEyeTracking();
	CloseClient();
	FreeXvisioDLL();
	Super::Deinitialize();
}

bool UXVisioStreamWorldSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return true;
}

// ---- Tick ----

void UXVisioStreamWorldSubsystem::Tick(float DeltaTime)
{
}

bool UXVisioStreamWorldSubsystem::IsTickable() const
{
	return GetWorld() && GetWorld()->IsGameWorld();
}

TStatId UXVisioStreamWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UXVisioStreamWorldSubsystem, STATGROUP_Tickables);
}

// ---- DLL / Client ----

void UXVisioStreamWorldSubsystem::ResolveDllExports()
{
	if (!XvisioDLL) return;

	CreateClientFunc = (CreateClientAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("CreateClientAPI"));
	CreateClientWithIpFunc = (CreateClientWithIpAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("CreateClientWithIpAPI"));
	SendMessageFunc = (SendMessageAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("SendMessageAPI"));
	StartRecordFunc = (StartRecordAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("startRecord"));
	ScreenShotFunc = (ScreenShotAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("screenShort"));
	StopRecordFunc = (StopRecordAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("stopRecord"));
	StartCSLAMMapFunc = (StartCSLAMMapAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("startCslamMap"));
	SetMapPathFunc = (SetMapPathAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("setMapPath"));
	SaveSlamMapFunc = (SaveSlamMapAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("SaveSlamMap"));
	SendMapFileFunc = (SendMapFileAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("sendMapFile"));
	StartASRFunc = (StartASRAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("startASR"));
	StartEyeTrackingFunc = (StartEyeTrackingAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("startEyeTracking"));
	GetEyeReceivedDataFunc = (GetEyeReceivedDataAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("getEyeReceivedData"));
	GetReceivedMessageFunc = (GetReceivedMessageAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("GetReceivedMessageAPI"));
	CloseClientFunc = (CloseClientAPI)FPlatformProcess::GetDllExport(XvisioDLL, TEXT("CloseClient"));
}

void UXVisioStreamWorldSubsystem::LoadXvisioDLL(const FString& ClientIP)
{
	FString DllPath;

#if PLATFORM_WINDOWS
	DllPath = FPaths::ProjectDir() / TEXT("Binaries/Win64/xv_client.dll");
#elif PLATFORM_LINUX
	DllPath = FPaths::ProjectDir() / TEXT("Binaries/Linux/libxv_client.so");
#else
	UE_LOG(LogTemp, Warning, TEXT("Unsupported platform"));
#endif

	UE_LOG(LogTemp, Log, TEXT("xv_client DLL dir is %s"), *FPaths::ProjectDir());
	if (DllPath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("xv_client DLL path is empty!"));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("xv_client DLL Path: %s"), *DllPath);

	XvisioDLL = FPlatformProcess::GetDllHandle(*DllPath);
	if (XvisioDLL)
	{
		UE_LOG(LogTemp, Log, TEXT("Loaded xv_client.dll success"));
		ResolveDllExports();

		int clientId = CreateClient();
		clientId = CreateClientWithIp(ClientIP);
#if PLATFORM_LINUX
		UE_LOG(LogTemp, Log, TEXT("xv_client ip is: %s"), *ClientIP);
		clientId = CreateClientWithIp(ClientIP);
#endif
		
		mClientId = clientId;
		UE_LOG(LogTemp, Warning, TEXT("xv_client mClientId: %d. if is -1 mean client disconnect"), mClientId);
		isDllLoaded = true;
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load xv_client.dll"));
	}
}

void UXVisioStreamWorldSubsystem::FreeXvisioDLL()
{
	if (XvisioDLL)
	{
		FPlatformProcess::FreeDllHandle(XvisioDLL);
		XvisioDLL = nullptr;
	}
	isDllLoaded = false;
}

int32 UXVisioStreamWorldSubsystem::CreateClient()
{
	if (CreateClientFunc)
	{
		UE_LOG(LogTemp, Log, TEXT("CreateClientFunc is not null"));
		return CreateClientFunc();
	}
	return -1;
}

int32 UXVisioStreamWorldSubsystem::CreateClientWithIp(const FString& ip)
{
	if (CreateClientWithIpFunc)
	{
		UE_LOG(LogTemp, Log, TEXT("CreateClientWithIpFunc is not null"));
		return CreateClientWithIpFunc(TCHAR_TO_ANSI(*ip));
	}
	return -1;
}

void UXVisioStreamWorldSubsystem::CloseClient()
{
	if (CloseClientFunc)
	{
		CloseClientFunc(mClientId);
	}
}

// ---- Eye Tracking ----

GetEyeReceivedDataAPI UXVisioStreamWorldSubsystem::GetEyeGazeAPI() const
{
	return GetEyeReceivedDataFunc;
}

StartEyeTrackingAPI UXVisioStreamWorldSubsystem::GetStartEyeTrackingAPI() const
{
	return StartEyeTrackingFunc;
}

void UXVisioStreamWorldSubsystem::StartEyeTracking()
{
	if (bIsEyeTrackingStarted)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartEyeTracking: 眼动已启动，跳过重复调用"));
		return;
	}

	if (StartEyeTrackingFunc)
	{
		UE_LOG(LogTemp, Log, TEXT("StartEyeTrackingFunc entry"));
		StartEyeTrackingFunc(mClientId);
	}

	EyeGazeDataRunnable::Configure(GetEyeReceivedDataFunc, StartEyeTrackingFunc, &mClientId, &isDllLoaded);
	EyeGazeDataRunnable* GazeRunnable = EyeGazeDataRunnable::GetInstance();
	if (GazeRunnable != nullptr)
	{
		bIsEyeTrackingStarted = true;
		UE_LOG(LogTemp, Log, TEXT("眼动线程启动成功"));
	}
}

void UXVisioStreamWorldSubsystem::EndEyeTracking()
{
	EyeGazeDataRunnable::DestroyInstance();
	bIsEyeTrackingStarted = false;
	UE_LOG(LogTemp, Log, TEXT("眼动线程关闭成功"));
}

bool UXVisioStreamWorldSubsystem::GetEyeReceivedData(FEyeData& OutEyeData)
{
	EyeGazeDataRunnable* GazeRunnable = EyeGazeDataRunnable::GetInstance();
	if (GazeRunnable != nullptr)
	{
		if (GazeRunnable->TryGetGazeData(OutEyeData))
		{
			UE_LOG(LogTemp, Log, TEXT("收到眼动数据：%s"), *OutEyeData.Position.ToString());
			return true;
		}
	}
	return false;
}

// ---- ASR ----

GetReceivedMessageAPI UXVisioStreamWorldSubsystem::GetASRMessageCallback() const
{
	return GetReceivedMessageFunc;
}

void UXVisioStreamWorldSubsystem::StartASR(const FString& bnf)
{
	if (StartASRFunc)
	{
		FTCHARToUTF8 UTF8String(*bnf);
		const char* BnfCharPtr = UTF8String.Get();
		UE_LOG(LogTemp, Log, TEXT("StartASR -> UTF8: %hs"), UTF8String.Get());
		StartASRFunc(mClientId, BnfCharPtr);
	}

	ASRDataRunNable::Configure(GetReceivedMessageFunc, &mClientId, &isDllLoaded);
	if (ASRDataRunNable::GetInstance() != nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("语音线程启动成功"));
	}
}

FString UXVisioStreamWorldSubsystem::GetReceivedMessage()
{
	ASRDataRunNable* ASRRunnable = ASRDataRunNable::GetInstance();
	if (ASRRunnable != nullptr)
	{
		FString message;
		while (ASRRunnable->TryGetMessageData(message))
		{
			UE_LOG(LogTemp, Log, TEXT("收到语音数据：%s"), *message);
		}
		return message;
	}
	return "";
}

void UXVisioStreamWorldSubsystem::EndASR()
{
	ASRDataRunNable::DestroyInstance();
	UE_LOG(LogTemp, Log, TEXT("语音线程关闭成功"));
}

// ---- SLAM Map ----

void UXVisioStreamWorldSubsystem::StartCSLAMMap()
{
	if (StartCSLAMMapFunc)
	{
		StartCSLAMMapFunc(mClientId);
	}
}

void UXVisioStreamWorldSubsystem::SetMapPath(const FString& filePath)
{
	if (SetMapPathFunc)
	{
		SetMapPathFunc(mClientId, TCHAR_TO_ANSI(*filePath));
	}
}

void UXVisioStreamWorldSubsystem::SaveSlamMap(const FString& pcPath, const FString& fileName)
{
	if (SaveSlamMapFunc)
	{
		SaveSlamMapFunc(mClientId, TCHAR_TO_ANSI(*pcPath), TCHAR_TO_ANSI(*fileName));
	}
}

void UXVisioStreamWorldSubsystem::SendMapFile(const FString& filePath)
{
	if (SendMapFileFunc)
	{
		SendMapFileFunc(mClientId, TCHAR_TO_ANSI(*filePath));
	}
}

// ---- AprilTag ----

bool UXVisioStreamWorldSubsystem::GetApriltag(int32& OutTagCount)
{
	OutTagCount = 0;

	if (!isDllLoaded || mClientId == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetApriltag(Stream): DLL未加载或ClientId无效"));
		return false;
	}

	// 统一数据流：眼动和 tag 由 EyeGazeDataRunnable 一并拉取
	EyeGazeDataRunnable* Runnable = EyeGazeDataRunnable::GetInstance();
	if (!Runnable)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetApriltag(Stream): EyeGazeDataRunnable 未启动，请先调用 StartEyeTracking"));
		return false;
	}

	{
		FScopeLock Lock(&Runnable->TagsLock);
		OutTagCount = Runnable->LatestTagCount;
	}

	return true;
}

bool UXVisioStreamWorldSubsystem::GetApriltagData(int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence)
{
	// 统一数据流：从 EyeGazeDataRunnable 读取 tag 数据
	EyeGazeDataRunnable* Runnable = EyeGazeDataRunnable::GetInstance();
	if (!Runnable)
		return false;

	{
		FScopeLock Lock(&Runnable->TagsLock);
		if (Index < 0 || Index >= Runnable->LatestTags.Num())
			return false;
		

		const FApriltagStreamData& Data = Runnable->LatestTags[Index];
		OutTagID      = Data.TagID;
		OutConfidence = Data.Confidence;

		// ---- 数据是世界坐标(s_pose * d.transform), 直接轴映射 ----
		float WorldToMetersScale = 1.0f;
		if (XRTrackingSystem)
		{
			WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();
		}

		float offset  = -0.0f;
		// Unity→UE 轴映射: (x,y,z)→(z,x,y)
		OutPosition    = FVector(Data.Position.Z * WorldToMetersScale - offset , Data.Position.X * WorldToMetersScale - offset, Data.Position.Y * WorldToMetersScale - offset);
		OutOrientation = FRotator(
			FMath::RadiansToDegrees(Data.Orientation.Z),
			FMath::RadiansToDegrees(Data.Orientation.X),
			FMath::RadiansToDegrees(Data.Orientation.Y));
		OutQuaternion  = FQuat(Data.Quaternion.Z, Data.Quaternion.X, Data.Quaternion.Y, Data.Quaternion.W);
	}
	return true;
}

void UXVisioStreamWorldSubsystem::StopApriltag()
{
	// 统一数据流下，tag 随 EyeGazeDataRunnable 一并启停
	// 如需停止 tag 数据，调用 EndEyeTracking() 即可
	UE_LOG(LogTemp, Log, TEXT("StopApriltag(Stream): 统一数据流，tag 随 EndEyeTracking 统一停止"));
}

// ---- Utility ----

void UXVisioStreamWorldSubsystem::ParseAsrResultMessage(const FString& MsgJsonString)
{
	TSharedPtr<FJsonObject> MsgJsonObject;
	TSharedRef<TJsonReader<>> MsgReader = TJsonReaderFactory<>::Create(MsgJsonString);

	if (!FJsonSerializer::Deserialize(MsgReader, MsgJsonObject) || !MsgJsonObject.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to parse 'msg' as JSON: %s"), *MsgJsonString);
		return;
	}

	const TArray<TSharedPtr<FJsonValue>>* WsArray;
	if (MsgJsonObject->TryGetArrayField(TEXT("ws"), WsArray))
	{
		for (const TSharedPtr<FJsonValue>& WsElement : *WsArray)
		{
			const TSharedPtr<FJsonObject>* WsObject;
			if (WsElement->TryGetObject(WsObject) && WsObject->IsValid())
			{
				const TArray<TSharedPtr<FJsonValue>>* CwArray;
				if ((*WsObject)->TryGetArrayField(TEXT("cw"), CwArray))
				{
					for (const TSharedPtr<FJsonValue>& CwElement : *CwArray)
					{
						const TSharedPtr<FJsonObject>* CwObject;
						if (CwElement->TryGetObject(CwObject) && CwObject->IsValid())
						{
							FString id;
							if ((*CwObject)->TryGetStringField(TEXT("id"), id))
							{
								GLog->Serialize(*id, ELogVerbosity::Log, FName(TEXT("LogTemp")));
							}
						}
					}
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("'ws' field missing in msg JSON"));
	}
}