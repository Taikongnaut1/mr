#pragma once
#include "IOpenXRExtensionPlugin.h"
#include "ExtensionsHead/XvisioOpenXRExtensions.h"


namespace XvisioOpenXR
{
	class FXvisioFunctionPlugin: public IOpenXRExtensionPlugin
	{
	public:
		FXvisioFunctionPlugin();
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

		bool getApriltag(unsigned char* tagFamily, double size, XrAprilTagDataXV* tagsArray, uint32_t* tagSize, uint32_t arraySize, uint32_t type);
		bool StopApriltag(uint32_t ret);

		bool GetCslamCallbackData(XrCSlamCallbackDataXV* callbackData);
		bool LoadMapAndSwitchToCslam(const FString& mapPath);
		bool SaveMapAndSwitchToCslam(const FString& mapPath);

	private:

		XrSession Session;
		PFN_xrDetectTags xrDetectTags_Func;
		PFN_xrStopDetectTags xrStopDetectTags_Func;

		PFN_xrGetCslamCallbackData xrGetCslamCallbackData_Func;
		PFN_xrLoadMapAndSwitchToCslamXVISIO xrLoadMapAndSwitchToCslam_Func;
		PFN_xrSaveMapAndSwitchToCslamXVISIO xrSaveMapAndSwitchToCslam_Func;
		
	};
}
