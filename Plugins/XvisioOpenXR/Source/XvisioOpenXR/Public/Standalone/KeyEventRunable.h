#pragma once
#include "Standalone/KeyEventPlugin.h"
#include "HAL/CriticalSection.h"
#include "HAL/Runnable.h"

DECLARE_MULTICAST_DELEGATE(FKeyEventDownDelegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventDownTickDelegate_Internal);
DECLARE_MULTICAST_DELEGATE(FKeyEventUpDelegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventMinusDelegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventPlusDelegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventDir1Delegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventDir2Delegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventDir3Delegate);
DECLARE_MULTICAST_DELEGATE(FKeyEventDir4Delegate);

//非串流的子线程用于轮寻获取按键事件
namespace XvisioOpenXR
{
	class AKeyEventRunable  : public FRunnable
	{
	private:
		// 单例核心成员
		static AKeyEventRunable* Instance; // 全局唯一实例
		static FCriticalSection SingletonLock; // 单例创建/销毁的互斥锁
		static bool bIsInstanceValid; // 实例有效性标记（避免重复销毁）
	
		// 线程相关成员
		FRunnableThread* Thread = nullptr; // 线程对象
		std::atomic<bool> bIsRunning = false; // 线程运行标记（控制 Run() 循环）
	
		//按键事件的对象
		XvisioOpenXR::FKeyEventPlugin* KeyEventPlugin;
	
		//按键的事件的数值
		XrEventData eventData;
		int previousRetGetEvent = 0;
		int e_type = 0;
		int e_state = 0;
	
		// 禁止拷贝/赋值
		AKeyEventRunable(const AKeyEventRunable&) = delete;
		AKeyEventRunable& operator=(const AKeyEventRunable&) = delete;

	public:
		AKeyEventRunable(XvisioOpenXR::FKeyEventPlugin* keyEventPlugin);
		
		XrEventData previousRetGetEventData;

		FEvent* TickEvent;
	
		FKeyEventDownDelegate KeyEventDownDelegate;
		
		FKeyEventDownTickDelegate_Internal KeyEventDownTickDelegate;
		
		FKeyEventUpDelegate KeyEventUpDelegate;
	
		FKeyEventMinusDelegate KeyEventMinusDelegate;
		
		FKeyEventPlusDelegate KeyEventPlusDelegate;

		FKeyEventDir1Delegate KeyEventDir1DownDelegate;
		FKeyEventDir1Delegate KeyEventDir1UpDelegate;

		FKeyEventDir2Delegate KeyEventDir2DownDelegate;
		FKeyEventDir2Delegate KeyEventDir2UpDelegate;

		FKeyEventDir3Delegate KeyEventDir3DownDelegate;
		FKeyEventDir3Delegate KeyEventDir3UpDelegate;

		FKeyEventDir4Delegate KeyEventDir4DownDelegate;
		FKeyEventDir4Delegate KeyEventDir4UpDelegate;

		static AKeyEventRunable* GetInstance(XvisioOpenXR::FKeyEventPlugin* keyEventPlugin);
		static void DestroyInstance();

		/** 线程安全地获取最新事件数据 */
		XrEventData GetLatestEventData() const;

		virtual bool Init() override;
		virtual uint32 Run() override;
		virtual void Stop() override;
		virtual void Exit() override;
	
		bool IsRunning() const { return bIsRunning; }
	};
}
