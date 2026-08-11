#include "Standalone/KeyEventRunable.h"

#include "Async/Async.h"
#include "HAL/RunnableThread.h"


// 静态成员初始化
XvisioOpenXR::AKeyEventRunable* XvisioOpenXR::AKeyEventRunable::Instance = nullptr;
FCriticalSection XvisioOpenXR::AKeyEventRunable::SingletonLock;
bool XvisioOpenXR::AKeyEventRunable::bIsInstanceValid = false;

XvisioOpenXR::AKeyEventRunable* XvisioOpenXR::AKeyEventRunable::GetInstance(XvisioOpenXR::FKeyEventPlugin* keyEventPlugin)
{
	if (Instance == nullptr)
	{
		FScopeLock Lock(&SingletonLock); 
		if (Instance == nullptr)
		{
			Instance = new AKeyEventRunable(keyEventPlugin);
			bIsInstanceValid = true;
            
			if (Instance->Init())
			{
				Instance->Thread = FRunnableThread::Create(
					Instance, 
					TEXT("AKeyEventRunable"), 
					0, 
					TPri_Normal 
				);
			}
			else
			{
				// 初始化失败，直接销毁实例
				DestroyInstance();
			}
		}
	}
	return Instance;
}

void XvisioOpenXR::AKeyEventRunable::DestroyInstance()
{
	AKeyEventRunable* LocalInstance = nullptr;

	{
		FScopeLock Lock(&SingletonLock);
		if (!Instance || !bIsInstanceValid)
		{
			return;
		}

		bIsInstanceValid = false;
		LocalInstance = Instance;
		Instance = nullptr;
	}
	
	LocalInstance->Stop();

	if (LocalInstance->Thread)
	{
		LocalInstance->Thread->WaitForCompletion();
		delete LocalInstance->Thread;
	}

	FPlatformProcess::ReturnSynchEventToPool(LocalInstance->TickEvent);
	delete LocalInstance;
}

bool XvisioOpenXR::AKeyEventRunable::Init()
{
	TickEvent = FPlatformProcess::GetSynchEventFromPool();
	if (!KeyEventPlugin && !TickEvent)
	{
		UE_LOG(LogTemp, Error, TEXT("AKeyEventRunable 初始化失败：事件接口"));
		return false; // 初始化失败，阻止线程启动
	}
	check(TickEvent);
	bIsRunning = true;
	UE_LOG(LogTemp, Log, TEXT("AKeyEventRunable init success"));
	return true; 
}

uint32 XvisioOpenXR::AKeyEventRunable::Run()
{
	UE_LOG(LogTemp, Log, TEXT("***************************AKeyEventRunable tick "));
	while (bIsRunning)
	{
		TickEvent->Wait();   // 等 

		UE_LOG(LogTemp, Log, TEXT("-------------------AKeyEventRunable tick "));
		
		int ret = -1;
		bool isOK = KeyEventPlugin->GetEventData(&eventData,&ret);
		if (isOK)
		{
			previousRetGetEvent = ret;
			
			e_type = eventData.eventType;
			e_state = eventData.eventState;
			
			UE_LOG(LogTemp, Log, TEXT("AKeyEventRunable  ret is: %d  e_type is：%d e_state is: %d"),ret ,e_type ,e_state);
			
			if (ret == 1)
			{
				switch (eventData.eventType)
				{
				case 1:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 254)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir1DownDelegate.Broadcast(); });
					else if (eventData.eventState == 255)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir1UpDelegate.Broadcast(); });
					break;
				case 2:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 254)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir2DownDelegate.Broadcast(); });
					else if (eventData.eventState == 255)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir2UpDelegate.Broadcast(); });
					break;
				case 3:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 254)
					{
						 AsyncTask(ENamedThreads::GameThread, [this]() {KeyEventDownDelegate.Broadcast();});
					}
					else if (eventData.eventState == 255)
					{
						AsyncTask(ENamedThreads::GameThread, [this]() {KeyEventUpDelegate.Broadcast();});
					}
					break;
				case 4:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 254)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir3DownDelegate.Broadcast(); });
					else if (eventData.eventState == 255)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir3UpDelegate.Broadcast(); });
					break;
				case 5:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 254)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir4DownDelegate.Broadcast(); });
					else if (eventData.eventState == 255)
						AsyncTask(ENamedThreads::GameThread, [this]() { KeyEventDir4UpDelegate.Broadcast(); });
					break;
				case 17:
					previousRetGetEventData = eventData;
					if (eventData.eventState == 101)
					{
						AsyncTask(ENamedThreads::GameThread, [this]() {KeyEventMinusDelegate.Broadcast();});
					}
					else if (eventData.eventState == 99)
					{
						AsyncTask(ENamedThreads::GameThread, [this]() {KeyEventPlusDelegate.Broadcast();});
					}
					break;
				
				}
			}
			else
			{
				switch (eventData.eventType)
				{
				case 0:
					if (previousRetGetEventData.eventState == 254)
					{
						AsyncTask(ENamedThreads::GameThread, [this]() {KeyEventDownTickDelegate.Broadcast();});
					}
					break;
				}
			}

		}
	}
	return 0;
}

void XvisioOpenXR::AKeyEventRunable::Stop()
{
	bIsRunning = false;
	TickEvent->Trigger();
	FRunnable::Stop();
}

void XvisioOpenXR::AKeyEventRunable::Exit()
{
	FRunnable::Exit();
}

XvisioOpenXR::AKeyEventRunable::AKeyEventRunable(XvisioOpenXR::FKeyEventPlugin* keyEventPlugin)
{
	KeyEventPlugin = keyEventPlugin;
	
	return;
}

XrEventData XvisioOpenXR::AKeyEventRunable::GetLatestEventData() const
{
	return previousRetGetEventData;
}
