/*
 * 串流类型定义 — FEyeData、DLL 函数指针 typedef
 * UXvisioStream 类为静态包装器，实际逻辑委托给 UXVisioStreamWorldSubsystem
 */

#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "XvisioStream.generated.h"

USTRUCT(BlueprintType)
struct XVISIOOPENXR_API FEyeData
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector PositionEnd = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct XVISIOOPENXR_API FApriltagStreamData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 TagID = -1;

	UPROPERTY(BlueprintReadOnly)
	FVector Position = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector Orientation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly)
	FVector4 Quaternion = FVector4(0, 0, 0, 1);

	UPROPERTY(BlueprintReadOnly)
	float Confidence = 0.0f;
};

// DLL 函数指针类型
typedef int(*CreateClientAPI)();
typedef int(*CreateClientWithIpAPI)(const char* ip);
typedef void(*SendMessageAPI)(int ClientId, const char* action, const char* message);
typedef void(*ScreenShotAPI)(int ClientId);
typedef void(*StartRecordAPI)(int ClientId);
typedef void(*StopRecordAPI)(int ClientId);
typedef void(*StartCSLAMMapAPI)(int ClientId);
typedef void(*SetMapPathAPI)(int ClientId, const char* filePath);
typedef void(*SaveSlamMapAPI)(int ClientId, const char* pcPath, const char* fileName);
typedef void(*SendMapFileAPI)(int ClientId, const char* filePath);
typedef void(*StartASRAPI)(int ClientId, const char* bnf);
typedef void(*StartEyeTrackingAPI)(int ClientId);
typedef const char*(*GetEyeReceivedDataAPI)();
typedef const char*(*GetReceivedMessageAPI)(int ClientId);
typedef void(*CloseClientAPI)(int ClientId);


UCLASS()
class XVISIOOPENXR_API UXvisioStream : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static int32 CreateClient(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static int32 CreateClientWithIp(UObject* WorldContextObject, const FString& ip);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void LoadXvisioDLL(UObject* WorldContextObject, const FString& ClientIP);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void FreeXvisioDLL(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void StartEyeTracking(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void EndEyeTracking(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static bool GetEyeReceivedData(UObject* WorldContextObject, FEyeData& OutEyeData);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void StartASR(UObject* WorldContextObject, const FString& bnf);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static FString GetReceivedMessage(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void EndASR(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void CloseClient(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming")
	static void ParseAsrResultMessage(const FString& MsgJsonString);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void StartCSLAMMap(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void SetMapPath(UObject* WorldContextObject, const FString& filePath);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void SaveSlamMap(UObject* WorldContextObject, const FString& pcPath, const FString& fileName);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void SendMapFile(UObject* WorldContextObject, const FString& filePath);

	// ---- AprilTag (统一数据流，由 EyeGazeDataRunnable 一并拉取) ----
	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static bool GetApriltag(UObject* WorldContextObject, int32& OutTagCount);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static bool GetApriltagData(UObject* WorldContextObject, int32 Index, int32& OutTagID, FVector& OutPosition, FRotator& OutOrientation, FQuat& OutQuaternion, float& OutConfidence);

	UFUNCTION(BlueprintCallable, Category = "Xvisio|Streaming", meta = (WorldContext = "WorldContextObject"))
	static void StopApriltag(UObject* WorldContextObject);
};