#include "Streaming/EyeGazeDataRunnable.h"
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/RunnableThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"


EyeGazeDataRunnable* EyeGazeDataRunnable::Instance = nullptr;
FCriticalSection EyeGazeDataRunnable::SingletonLock;
bool EyeGazeDataRunnable::bIsInstanceValid = false;

GetEyeReceivedDataAPI EyeGazeDataRunnable::PendingEyeFunc = nullptr;
StartEyeTrackingAPI EyeGazeDataRunnable::PendingStartFunc = nullptr;
int* EyeGazeDataRunnable::PendingClientId = nullptr;
bool* EyeGazeDataRunnable::PendingDllLoaded = nullptr;

void EyeGazeDataRunnable::Configure(GetEyeReceivedDataAPI InEyeFunc, StartEyeTrackingAPI InStartFunc, int* InClientId, bool* InDllLoaded)
{
	PendingEyeFunc = InEyeFunc;
	PendingStartFunc = InStartFunc;
	PendingClientId = InClientId;
	PendingDllLoaded = InDllLoaded;
}


EyeGazeDataRunnable::EyeGazeDataRunnable()
{
}

EyeGazeDataRunnable::~EyeGazeDataRunnable()
{
	if (Thread != nullptr)
	{
		EyeGazeDataRunnable::Stop();
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

EyeGazeDataRunnable* EyeGazeDataRunnable::GetInstance()
{
	if (Instance == nullptr)
	{
		FScopeLock Lock(&SingletonLock);
		if (Instance == nullptr)
		{
			Instance = new EyeGazeDataRunnable();
			bIsInstanceValid = true;

			if (Instance->Init())
			{
				Instance->Thread = FRunnableThread::Create(
					Instance,
					TEXT("EyeGazeThread"),
					0,
					TPri_Normal
				);
			}
			else
			{
				DestroyInstance();
			}
		}
	}
	return Instance;
}

void EyeGazeDataRunnable::DestroyInstance()
{
	FScopeLock Lock(&SingletonLock);
	if (Instance != nullptr && bIsInstanceValid)
	{
		bIsInstanceValid = false;
		Instance->Stop();
		if (Instance->Thread != nullptr)
		{
			Instance->Thread->WaitForCompletion();
			delete Instance->Thread;
			Instance->Thread = nullptr;
		}
		// 清理 tag 数据
		{
			FScopeLock TagLock(&Instance->TagsLock);
			Instance->LatestTags.Empty();
			Instance->LatestTagCount = 0;
		}
		delete Instance;
		Instance = nullptr;
	}
	PendingEyeFunc = nullptr;
	PendingStartFunc = nullptr;
	PendingClientId = nullptr;
	PendingDllLoaded = nullptr;
}

bool EyeGazeDataRunnable::Init()
{
	if (!PendingStartFunc || !PendingEyeFunc || !PendingClientId || !PendingDllLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("EyeGazeDataRunnable 初始化失败：未通过 Configure 设置函数接口"));
		return false;
	}
	GetEyeReceivedDataFunc = PendingEyeFunc;
	StartEyeTrackingFunc = PendingStartFunc;
	mClientIdPtr = PendingClientId;
	mDllLoadedPtr = PendingDllLoaded;

	bIsRunning = true;
	UE_LOG(LogTemp, Log, TEXT("GetEyeGazeDataRunnable init success"));
	return true;
}

uint32 EyeGazeDataRunnable::Run()
{
	while (bIsRunning && bIsInstanceValid)
	{
		FEyeData OutEyeData;

		if (!IsValid(GWorld) || !mDllLoadedPtr || !*mDllLoadedPtr || !mClientIdPtr || *mClientIdPtr == -1)
		{
			UE_LOG(LogTemp, Warning, TEXT("世界实例或 XvisioCore 已销毁，线程准备退出"));
			bIsRunning = false;
			break;
		}

		if (!GetEyeReceivedDataFunc)
		{
			UE_LOG(LogTemp, Log, TEXT("---------------未获取到眼动的函数接口--------------------"));
			FPlatformProcess::Sleep(0.01f);
			continue;
		}

		if (StartEyeTrackingFunc && GetEyeReceivedDataFunc && mDllLoadedPtr && *mDllLoadedPtr)
		{
			UE_LOG(LogTemp, Log, TEXT("---------------后台线程读取眼动数据--------------------"));
			const char* result = GetEyeReceivedDataFunc();
			UE_LOG(LogTemp, Log, TEXT("---------------获取到的数据如下：%hs"), result);
			if (!result || strlen(result) < 10)
			{
				FPlatformProcess::Sleep(0.01f);
				continue;
			}

			FString ResultString = FString(UTF8_TO_TCHAR(result));
			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResultString);

			if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
			{
				UE_LOG(LogTemp, Error, TEXT("JSON parse failed: %s"), *ResultString);
				FPlatformProcess::Sleep(0.01f);
				continue;
			}

			const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
			const TArray<TSharedPtr<FJsonValue>>* OrientationArray = nullptr;

			if (!JsonObject->TryGetArrayField(TEXT("position"), PositionArray) ||
				!JsonObject->TryGetArrayField(TEXT("orientation"), OrientationArray))
			{
				UE_LOG(LogTemp, Error, TEXT("Missing JSON fields"));
				FPlatformProcess::Sleep(0.01f);
				continue;
			}

			if (!PositionArray || PositionArray->Num() < 3 || !OrientationArray || OrientationArray->Num() < 4)
			{
				UE_LOG(LogTemp, Error, TEXT("Invalid array size"));
				FPlatformProcess::Sleep(0.01f);
				continue;
			}
			UE_LOG(LogTemp, Log, TEXT("---------------眼动数据已经获取到了--------------------"));
			OutEyeData.Position = FVector(
				(float)(*PositionArray)[0]->AsNumber(),
				(float)(*PositionArray)[1]->AsNumber(),
				(float)(*PositionArray)[2]->AsNumber() + 150
			);

			OutEyeData.PositionEnd = FVector(
				(float)(*OrientationArray)[1]->AsNumber(),
				(float)(*OrientationArray)[0]->AsNumber(),
				(float)(*OrientationArray)[2]->AsNumber()
			);

			UE_LOG(LogTemp, Log, TEXT("Eye Position: X=%.3f, Y=%.3f, Z=%.3f"),
				   OutEyeData.Position.X, OutEyeData.Position.Y, OutEyeData.Position.Z);
			UE_LOG(LogTemp, Log, TEXT("Eye Orientation: X=%.3f, Y=%.3f, Z=%.3f"),
				   OutEyeData.PositionEnd.X, OutEyeData.PositionEnd.Y,
				   OutEyeData.PositionEnd.Z);

			GazeDataQueue.Enqueue(OutEyeData);

			// ---- 从同一 JSON 中解析 AprilTag 数据 ----
			const TArray<TSharedPtr<FJsonValue>>* TagsArray = nullptr;
			if (JsonObject->TryGetArrayField(TEXT("tags"), TagsArray) && TagsArray)
			{
				FScopeLock Lock(&TagsLock);
				LatestTags.Empty();
				LatestTagCount = TagsArray->Num();

				for (const TSharedPtr<FJsonValue>& TagValue : *TagsArray)
				{
					const TSharedPtr<FJsonObject>* TagObj = nullptr;
					if (!TagValue->TryGetObject(TagObj) || !TagObj->IsValid())
						continue;

					FApriltagStreamData Data;
					// 统一 JSON 中字段名为 tagID
					(*TagObj)->TryGetNumberField(TEXT("tagID"), Data.TagID);
					(*TagObj)->TryGetNumberField(TEXT("confidence"), Data.Confidence);

					const TArray<TSharedPtr<FJsonValue>>* PosArr = nullptr;
					if ((*TagObj)->TryGetArrayField(TEXT("position"), PosArr) && PosArr->Num() >= 3)
					{
						Data.Position = FVector(
							(float)(*PosArr)[0]->AsNumber(),
							(float)(*PosArr)[1]->AsNumber(),
							(float)(*PosArr)[2]->AsNumber()
						);
					}

					const TArray<TSharedPtr<FJsonValue>>* OriArr = nullptr;
					if ((*TagObj)->TryGetArrayField(TEXT("orientation"), OriArr) && OriArr->Num() >= 3)
					{
						Data.Orientation = FVector(
							(float)(*OriArr)[0]->AsNumber(),
							(float)(*OriArr)[1]->AsNumber(),
							(float)(*OriArr)[2]->AsNumber()
						);
					}

					const TArray<TSharedPtr<FJsonValue>>* QuatArr = nullptr;
					if ((*TagObj)->TryGetArrayField(TEXT("quaternion"), QuatArr) && QuatArr->Num() >= 4)
					{
						Data.Quaternion = FVector4(
							(float)(*QuatArr)[0]->AsNumber(),
							(float)(*QuatArr)[1]->AsNumber(),
							(float)(*QuatArr)[2]->AsNumber(),
							(float)(*QuatArr)[3]->AsNumber()
						);
					}

					LatestTags.Add(Data);
					TagDataQueue.Enqueue(Data);
				}
			}

			FPlatformProcess::Sleep(0.05f);
		}
		else
		{
			FPlatformProcess::Sleep(0.01f);
			continue;
		}

		if (!bIsRunning || !bIsInstanceValid)
		{
			break;
		}
	}
	return 0;
}


void EyeGazeDataRunnable::Stop()
{
	bIsRunning = false;
	UE_LOG(LogTemp, Log, TEXT("GetEyeGazeDataRunnable 收到停止信号"));
}


void EyeGazeDataRunnable::Exit()
{
	UE_LOG(LogTemp, Log, TEXT("GetEyeGazeDataRunnable 线程退出，清理资源"));
}

bool EyeGazeDataRunnable::TryGetGazeData(FEyeData& OutGazePoint)
{
	FEyeData CurrentData;
	bool bGotLatestData = false;
	while (GazeDataQueue.Dequeue(CurrentData))
	{
		bGotLatestData = true;
		OutGazePoint = CurrentData;
	}
	return bGotLatestData;
}

bool EyeGazeDataRunnable::TryGetTagData(FApriltagStreamData& OutData)
{
	FApriltagStreamData CurrentData;
	bool bGotLatestData = false;
	while (TagDataQueue.Dequeue(CurrentData))
	{
		bGotLatestData = true;
		OutData = CurrentData;
	}
	return bGotLatestData;
}