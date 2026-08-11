#pragma once
#include "Streaming/XvisioStream.h"
#include "Containers/Queue.h"
#include "Containers/UnrealString.h"
#include "HAL/Runnable.h"

class ASRDataRunNable: public FRunnable
{

private:
	static ASRDataRunNable* Instance;
	static FCriticalSection SingletonLock;
	static bool bIsInstanceValid;

	TQueue<FString, EQueueMode::Mpsc> MessageQueue;

	FRunnableThread* Thread = nullptr;
	bool bIsRunning = false;

	GetReceivedMessageAPI GetReceivedMessageFunc = nullptr;
	int* mClientIdPtr = nullptr;
	bool* mDllLoadedPtr = nullptr;

	ASRDataRunNable();
	~ASRDataRunNable() override;

	ASRDataRunNable(const ASRDataRunNable&) = delete;
	ASRDataRunNable& operator=(const ASRDataRunNable&) = delete;

	// ---- 由子系统在 GetInstance 前调用 ----
	static GetReceivedMessageAPI PendingFunc;
	static int* PendingClientId;
	static bool* PendingDllLoaded;

public:
	// 子系统必须在 GetInstance 之前调用，传入 DLL 函数指针和状态指针
	static void Configure(GetReceivedMessageAPI InFunc, int* InClientId, bool* InDllLoaded);

	static ASRDataRunNable* GetInstance();
	static void DestroyInstance();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	bool IsRunning() const { return bIsRunning; }

	bool TryGetMessageData(FString& MessageResult);
};