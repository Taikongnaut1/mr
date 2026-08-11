#pragma once
#include "Streaming/XvisioStream.h"
#include "Containers/Queue.h"
#include "HAL/Runnable.h"
#include "UObject/ObjectMacros.h"

struct FEyeData;
struct FApriltagStreamData;

class EyeGazeDataRunnable : public FRunnable
{

private:
	static EyeGazeDataRunnable* Instance;
	static FCriticalSection SingletonLock;
	static bool bIsInstanceValid;

	TQueue<FEyeData, EQueueMode::Mpsc> GazeDataQueue;
	TQueue<FApriltagStreamData, EQueueMode::Mpsc> TagDataQueue;

	FRunnableThread* Thread = nullptr;
	bool bIsRunning = false;

	GetEyeReceivedDataAPI GetEyeReceivedDataFunc = nullptr;
	StartEyeTrackingAPI StartEyeTrackingFunc = nullptr;
	int* mClientIdPtr = nullptr;
	bool* mDllLoadedPtr = nullptr;

	EyeGazeDataRunnable();
	~EyeGazeDataRunnable() override;

	EyeGazeDataRunnable(const EyeGazeDataRunnable&) = delete;
	EyeGazeDataRunnable& operator=(const EyeGazeDataRunnable&) = delete;

	// ---- 由子系统在 GetInstance 前调用 ----
	static GetEyeReceivedDataAPI PendingEyeFunc;
	static StartEyeTrackingAPI PendingStartFunc;
	static int* PendingClientId;
	static bool* PendingDllLoaded;

public:
	static void Configure(GetEyeReceivedDataAPI InEyeFunc, StartEyeTrackingAPI InStartFunc, int* InClientId, bool* InDllLoaded);

	static EyeGazeDataRunnable* GetInstance();
	static void DestroyInstance();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;
	virtual void Exit() override;

	bool IsRunning() const { return bIsRunning; }

	bool TryGetGazeData(FEyeData& OutGazePoint);
	bool TryGetTagData(FApriltagStreamData& OutData);

	// Unified tag storage (thread-safe via TagsLock)
	TArray<FApriltagStreamData> LatestTags;
	FCriticalSection TagsLock;
	int32 LatestTagCount = 0;
};