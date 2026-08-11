#pragma once

#include "IOpenXRExtensionPlugin.h"
#include "OpenXRCore.h"
#include "GameFramework/PlayerInput.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/InputSettings.h"
#include "ExtensionsHead/XvisioOpenXRExtensions.h"


namespace XvisioOpenXR
{
	class XVISIOOPENXR_API FGazeCalibrationPlugin : public IOpenXRExtensionPlugin
	{
	public:	
		
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
		
		
		
		//眼控制校准代码
		bool GazeSetConfigPath(const FString& InPath);
		bool StartGaze(int32_t* ret);
		bool StopGaze(int32_t* ret);
		bool SetExposure(int32_t leftGain, float leftTimeMs, int32_t rightGain, float rightTimeMs, int32_t* ret);
		bool SetBright(int32_t eye,int32_t led,int32_t brightness,int32_t* ret);
		
		bool GetGazeCallback(XrEtEyeDataEx* data);

		bool ChangeIpd(float distance,int32_t* ret);
		bool CameraDistance(uint32_t distance);
		
		
		bool GetDeviceCameraImageData(unsigned char* data,uint32_t type, uint32_t width, uint32_t height,double* timestamp);
		bool GetDeviceCameraImageSize(uint32_t type, uint32_t* width, uint32_t* height);
		bool GazeCalibrationPubBegin(int32_t ETIndex, int32_t* ret);
		bool GazeCalibrationPubSetConfGaze(int32_t type, int32_t ETIndex,float x,float y,uint32_t* ret);
		bool GazeCalibrationPubEnd(int32_t ETIndex, int32_t* ret);
		bool SetGazeConfigs( xrGazeConfigsData* data, int32_t* ret);
		bool SetPref(int32_t etIdx, unsigned char* data, int32_t size, int32_t* ret);
		bool GetPref(int32_t etIdx, unsigned char* data, int32_t* size, int32_t* ret);
		bool GetHiValue(int32_t* ret);
		bool GetSrValue(int32_t* ret);
		bool GazeEnableDump(int32_t enable, int32_t* ret);
		bool SetUsrEyeReady(int32_t* ret);
		bool GetGazeIpd(float_t* ipd);
		bool StartEyetracking(int32_t* ret);
		bool StopEyetracking(int32_t* ret);
		
	private:
		
		XrSession Session;
		
		PFN_xrGazeSetConfigPath xrGazeSetConfigPath_Func = nullptr;
		PFN_xrStartGaze xrStartGaze_Func = nullptr;
		PFN_xrStopGaze xrStopGaze_Func = nullptr;
		PFN_xrSetExposure xrSetExposure_Func = nullptr;
		PFN_xrSetBright xrSetBright_Func = nullptr;
		
		PFN_xrGetGazeCallback xrGetGazeCallback_Func = nullptr;
		
		PFN_xrChangeIpd xrChangeIpd_Func = nullptr;
		PFN_xrCameraDistance xrCameraDistance_Func = nullptr;
		
		PFN_xrGetDeviceCameraImageData xrGetDeviceCameraImageData_Func = nullptr;
		PFN_xrGetDeviceCameraImageSize xrGetDeviceCameraImageSize_Func = nullptr;
		PFN_xrGazeCalibrationPubBegin xrGazeCalibrationPubBegin_Func = nullptr;
		PFN_xrGazeCalibrationPubSetConfGaze xrGazeCalibrationPubSetConfGaze_Func = nullptr;
		PFN_xrGazeCalibrationPubEnd xrGazeCalibrationPubEnd_Func = nullptr;
		PFN_xrSetGazeConfigs xrSetGazeConfigs_Func = nullptr;
		PFN_xrSetPref xrSetPref_Func = nullptr;
		PFN_xrGetPref xrGetPref_Func = nullptr;
		PFN_xrGetHiValue xrGetHiValue_Func = nullptr;
		PFN_xrGetSrValue xrGetSrValue_Func = nullptr;
		PFN_xrGazeEnableDump xrGazeEnableDump_Func = nullptr;
		PFN_xrSetUsrEyeReady xrSetUsrEyeReady_Func = nullptr;
		PFN_xrGetGazeIpd xrGetGazeIpd_Func = nullptr;
		PFN_xrStartEyetracking xrStartEyetracking_Func = nullptr;
		PFN_xrStopEyetracking xrStopEyetracking_Func = nullptr;
		
		float WorldToMetersScale = 100.0f;
	};
	
}