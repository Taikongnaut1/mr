#include "Streaming/ASRDataRunNable.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "HAL/RunnableThread.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UXVisioStreamWorldSubsystem.h"


ASRDataRunNable* ASRDataRunNable::Instance = nullptr;
FCriticalSection ASRDataRunNable::SingletonLock;
bool ASRDataRunNable::bIsInstanceValid = false;

GetReceivedMessageAPI ASRDataRunNable::PendingFunc = nullptr;
int* ASRDataRunNable::PendingClientId = nullptr;
bool* ASRDataRunNable::PendingDllLoaded = nullptr;

void ASRDataRunNable::Configure(GetReceivedMessageAPI InFunc, int* InClientId, bool* InDllLoaded)
{
	PendingFunc = InFunc;
	PendingClientId = InClientId;
	PendingDllLoaded = InDllLoaded;
}


ASRDataRunNable::ASRDataRunNable()
{
}

ASRDataRunNable::~ASRDataRunNable()
{
	if (Thread != nullptr)
	{
		ASRDataRunNable::Stop();
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}
}

ASRDataRunNable* ASRDataRunNable::GetInstance()
{
	if (Instance == nullptr)
	{
		FScopeLock Lock(&SingletonLock);
		if (Instance == nullptr)
		{
			Instance = new ASRDataRunNable();
			bIsInstanceValid = true;

			if (Instance->Init())
			{
				Instance->Thread = FRunnableThread::Create(
					Instance,
					TEXT("ASRDataThread"),
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

void ASRDataRunNable::DestroyInstance()
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
		delete Instance;
		Instance = nullptr;
	}
	PendingFunc = nullptr;
	PendingClientId = nullptr;
	PendingDllLoaded = nullptr;
}

bool ASRDataRunNable::Init()
{
	if (!PendingFunc || !PendingClientId || !PendingDllLoaded)
	{
		UE_LOG(LogTemp, Error, TEXT("ASRDataRunNable 初始化失败：未通过 Configure 设置函数接口"));
		return false;
	}
	GetReceivedMessageFunc = PendingFunc;
	mClientIdPtr = PendingClientId;
	mDllLoadedPtr = PendingDllLoaded;

	bIsRunning = true;
	UE_LOG(LogTemp, Log, TEXT("ASRDataRunNable init success"));
	return true;
}

uint32 ASRDataRunNable::Run()
{
	while (bIsRunning && bIsInstanceValid)
	{
		if (!IsValid(GWorld) || !mDllLoadedPtr || !*mDllLoadedPtr || !mClientIdPtr || *mClientIdPtr == -1)
		{
			UE_LOG(LogTemp, Warning, TEXT("世界实例或 XvisioCore 已销毁，线程准备退出"));
			bIsRunning = false;
			break;
		}

		FString MessageResult;
		if (!GetReceivedMessageFunc)
		{
			UE_LOG(LogTemp, Log, TEXT("---------------未获取到语音的函数接口--------------------"));
			FPlatformProcess::Sleep(0.05f);
			continue;
		}

		const char* result = GetReceivedMessageFunc(*mClientIdPtr);
		FString Message(result);
		if (Message == TEXT(""))
		{
			FPlatformProcess::Sleep(0.02f);
			continue;
		}

		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Message);

		UE_LOG(LogTemp, Log, TEXT("message is %s"), *Message);
		if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
		{
			FString ActionValue;
			if (JsonObject->TryGetStringField(TEXT("action"), ActionValue))
			{
				if (ActionValue.Equals(TEXT("loadMapDone")))
				{
					MessageQueue.Enqueue("Action is loadMapDone");
					UE_LOG(LogTemp, Log, TEXT("Action is loadMapDone"));
				}
				else if (ActionValue.Equals(TEXT("saveMapDone")))
				{
					MessageQueue.Enqueue("Action is saveMapDone ");
					UE_LOG(LogTemp, Log, TEXT("Action is saveMapDone"));
				}
				else if (ActionValue.Equals(TEXT("onKeyPressed")))
				{
					UE_LOG(LogTemp, Log, TEXT("Action is onKeyPressed"));
					FString RawMsgValue;
					if (JsonObject->TryGetStringField(TEXT("msg"), RawMsgValue))
					{
						UE_LOG(LogTemp, Log, TEXT("onKeyPressed msg (Raw): %s"), *RawMsgValue);
						MessageQueue.Enqueue("onKeyPressed msg (Raw): " + RawMsgValue);
					}
				}
				else if (ActionValue.Equals(TEXT("onSpeechStart")))
				{
					UE_LOG(LogTemp, Log, TEXT("Action is onSpeechStart"));
					MessageQueue.Enqueue("Action is onSpeechStart");
				}
				else if (ActionValue.Equals(TEXT("onSpeechEnd")))
				{
					UE_LOG(LogTemp, Log, TEXT("Action is onSpeechEnd"));
					MessageQueue.Enqueue("Action is onSpeechEnd");
				}
				else if (ActionValue.Equals(TEXT("onAsrNoResult")))
				{
					UE_LOG(LogTemp, Log, TEXT("Action is onAsrNoResult"));
					MessageQueue.Enqueue("Action is onAsrNoResult");
				}
				else if (ActionValue.Equals(TEXT("onAsrResult")))
				{
					FString RawMsgValue;
					if (JsonObject->TryGetStringField(TEXT("msg"), RawMsgValue))
					{
						int32 PipeIndex;
						if (RawMsgValue.FindChar('|', PipeIndex))
						{
							RawMsgValue = RawMsgValue.Left(PipeIndex);
						}

						FString DecodedMsgValue = RawMsgValue.Replace(TEXT("\\n"), TEXT("")).Replace(TEXT("\\\""), TEXT("\""));

						UE_LOG(LogTemp, Log, TEXT("onAsrResult msg (Raw): %s"), *RawMsgValue);
						UE_LOG(LogTemp, Log, TEXT("onAsrResult msg (Decoded): %s"), *DecodedMsgValue);

						UXVisioStreamWorldSubsystem::ParseAsrResultMessage(DecodedMsgValue);
						MessageQueue.Enqueue(DecodedMsgValue);
					}
					else
					{
						MessageQueue.Enqueue("'msg' field missing in onAsrResult JSON");
						UE_LOG(LogTemp, Warning, TEXT("'msg' field missing in onAsrResult JSON"));
					}
				}
				else
				{
					MessageQueue.Enqueue("Action is unknown:" + ActionValue);
					UE_LOG(LogTemp, Log, TEXT("Action is unknown: %s"), *ActionValue);
				}
			}
			else
			{
				MessageQueue.Enqueue("Failed to get 'action' field");
				UE_LOG(LogTemp, Warning, TEXT("Failed to get 'action' field"));
			}
		}
		else
		{
			MessageQueue.Enqueue("Failed to parse JSON");
			UE_LOG(LogTemp, Error, TEXT("Failed to parse JSON"));
		}

		FPlatformProcess::Sleep(0.05f);

		if (!bIsRunning || !bIsInstanceValid)
		{
			break;
		}
	}
	return 0;
}


void ASRDataRunNable::Stop()
{
	bIsRunning = false;
	UE_LOG(LogTemp, Log, TEXT("ASRDataRunNable 收到停止信号"));
}


void ASRDataRunNable::Exit()
{
	UE_LOG(LogTemp, Log, TEXT("ASRDataRunNable 线程退出，清理资源"));
}

bool ASRDataRunNable::TryGetMessageData(FString& MessageResult)
{
	FString CurrentData;
	bool bGotLatestData = false;
	while (MessageQueue.Dequeue(CurrentData))
	{
		bGotLatestData = true;
		MessageResult = CurrentData;
	}
	return bGotLatestData;
}