/*
 * 串流子系统 - 管理 xv_client.dll 生命周期、网络连接、眼动/ASR后台线程
 */
#pragma once

#include "CoreMinimal.h"
#include "IXRTrackingSystem.h"
#include "Streaming/XvisioStream.h"
#include "Subsystems/WorldSubsystem.h"
#include "UXVisioStreamWorldSubsystem.generated.h"

UCLASS()
class XVISIOOPENXR_API UXVisioStreamWorldSubsystem
	: public UWorldSubsystem
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// ---- Subsystem Lifecycle ----
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// ---- Tick ----
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	// ---- State (public for runnable access) ----
	int mClientId = -1;
	bool isDllLoaded = false;

private:
	bool bIsEyeTrackingStarted = false;

public:

	// ---- DLL / Client ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void LoadXvisioDLL(const FString& ClientIP);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void FreeXvisioDLL();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	int32 CreateClient();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	int32 CreateClientWithIp(const FString& ip);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void CloseClient();

	// ---- Eye Tracking ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void StartEyeTracking();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void EndEyeTracking();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	bool GetEyeReceivedData(FEyeData& OutEyeData);

	GetEyeReceivedDataAPI GetEyeGazeAPI() const;
	StartEyeTrackingAPI GetStartEyeTrackingAPI() const;

	// ---- ASR ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void StartASR(const FString& bnf);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	FString GetReceivedMessage();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void EndASR();

	GetReceivedMessageAPI GetASRMessageCallback() const;

	// ---- SLAM Map ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void StartCSLAMMap();

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void SetMapPath(const FString& filePath);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void SaveSlamMap(const FString& pcPath, const FString& fileName);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void SendMapFile(const FString& filePath);

	// ---- AprilTag (统一数据流，由 EyeGazeDataRunnable 一并拉取) ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	bool GetApriltag(int32& OutTagCount);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	bool GetApriltagData(int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	void StopApriltag();

	// ---- Utility ----
	static void ParseAsrResultMessage(const FString& MsgJsonString);

private:
	void* XvisioDLL = nullptr;

	// DLL function pointers
	CreateClientAPI CreateClientFunc = nullptr;
	CreateClientWithIpAPI CreateClientWithIpFunc = nullptr;
	SendMessageAPI SendMessageFunc = nullptr;
	ScreenShotAPI ScreenShotFunc = nullptr;
	StartRecordAPI StartRecordFunc = nullptr;
	StopRecordAPI StopRecordFunc = nullptr;
	StartCSLAMMapAPI StartCSLAMMapFunc = nullptr;
	SaveSlamMapAPI SaveSlamMapFunc = nullptr;
	SetMapPathAPI SetMapPathFunc = nullptr;
	SendMapFileAPI SendMapFileFunc = nullptr;
	StartASRAPI StartASRFunc = nullptr;
	StartEyeTrackingAPI StartEyeTrackingFunc = nullptr;
	GetEyeReceivedDataAPI GetEyeReceivedDataFunc = nullptr;
	GetReceivedMessageAPI GetReceivedMessageFunc = nullptr;
	CloseClientAPI CloseClientFunc = nullptr;

	IXRTrackingSystem* XRTrackingSystem = nullptr;

	void ResolveDllExports();
};