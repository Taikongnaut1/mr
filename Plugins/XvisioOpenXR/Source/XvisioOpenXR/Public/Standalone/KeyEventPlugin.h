#pragma once
#include "IOpenXRExtensionPlugin.h"
#include "ExtensionsHead/XvisioOpenXRExtensions.h"


namespace XvisioOpenXR
{
	class FKeyEventPlugin: public IOpenXRExtensionPlugin
	{
	public:
		FKeyEventPlugin();
		//插件生命周期
		void Register();
		void Unregister();
	private:
		void OnEndPlay();
		
		
	public:	
		//openxr扩展支持
		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		//在Session中获取函数接口
		virtual const void* OnCreateSession(XrInstance InInstance, XrSystemId InSystem, const void* InNext) override;
		virtual const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		
		bool StartEvent(int32_t* ret);
		bool GetEventData(XrEventData* eventData, int32_t* ret);
		bool StopEvent(int32_t* ret);

		bool DeviceRgbSetBrightnessXVISIO(uint32_t ret);
		
	private:
		
		XrSession Session;
		PFN_xrStartEvent xrStartEvent_Func;
		PFN_xrGetEventData xrGetEventData_Func ;
		PFN_xrStopEvent xrStopEvent_Func;

		PFN_xrDeviceRgbSetBrightnessXVISIO xrDeviceRgbSetBrightnessXVISIO_Func;
	
	};
}
