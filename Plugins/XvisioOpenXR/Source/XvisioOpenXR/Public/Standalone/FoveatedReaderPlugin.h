#pragma once
#include "IOpenXRExtensionPlugin.h"
#include "ExtensionsHead/XvisioOpenXRExtensions.h"


namespace XvisioOpenXR
{
	class FFoveatedReaderPlugin: public IOpenXRExtensionPlugin
	{
	public:
		FFoveatedReaderPlugin();
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
		

	
	};
}
