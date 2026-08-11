#include "Standalone/GazeCalibrationPlugin.h"
#include "GameDelegates.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "ExtensionsHead/XvisioOpenXRExtensions.h"
#include "HeadMountedDisplayTypes.h"
#include "IOpenXRHMDModule.h"
#include "Standalone/OpenXRCommon.h"
#include "Engine/World.h"


void XvisioOpenXR::FGazeCalibrationPlugin::Register()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FGazeCalibrationPlugin::OnEndPlay);
}

void XvisioOpenXR::FGazeCalibrationPlugin::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}


bool XvisioOpenXR::FGazeCalibrationPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	UE_LOG(LogTemp, Log, TEXT("FGazeCalibrationPlugin :  GetRequiredExtensions"));
	OutExtensions.Add(XR_XVISIO_GAZE_CALIBRATION_PUB_EXTENSION_NAME);
	return true;
}

void XvisioOpenXR::FGazeCalibrationPlugin::OnEndPlay()
{
	return;
}

const void* XvisioOpenXR::FGazeCalibrationPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem,
                                                                  const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("FGazeCalibrationPlugin :  OnCreateSession"));
	XR_ENSURE_MSFT(
		xrGetInstanceProcAddr(InInstance, "xrEnumerateInstanceExtensionProperties", (PFN_xrVoidFunction*)&
			xrEnumerateInstanceExtensionProperties));
	uint32_t ExtensionsCount = 0;
	if (XR_SUCCEEDED(xrEnumerateInstanceExtensionProperties(nullptr, 0, &ExtensionsCount, nullptr)))
	{
		TArray<XrExtensionProperties> Properties;
		Properties.SetNum(ExtensionsCount);
		for (auto& Prop : Properties)
		{
			Prop = XrExtensionProperties{XR_TYPE_EXTENSION_PROPERTIES};
		}
	
		if (XR_ENSURE_MSFT(
				xrEnumerateInstanceExtensionProperties(nullptr, ExtensionsCount, &ExtensionsCount, Properties.GetData()
				)))
		{
			int i = 0;
			for (const XrExtensionProperties& Prop : Properties)
			{
				i++;
				UE_LOG(LogTemp, Log, TEXT("Extension id is: %d  Name is: %s "), i, ANSI_TO_TCHAR(Prop.extensionName));
	
				// 这里可以检查扩展是否启用
				//IOpenXRHMDModule::Get().IsExtensionEnabled("XR_EXT_dpad_binding")
				if (IOpenXRHMDModule::Get().IsExtensionEnabled(Prop.extensionName))
				{
					UE_LOG(LogTemp, Log, TEXT("扩展 %s 已启用"), ANSI_TO_TCHAR(Prop.extensionName));
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("扩展 %s 未启用"), ANSI_TO_TCHAR(Prop.extensionName));
				}
			}
		}
	}
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGazeSetConfigPath", (PFN_xrVoidFunction*)&xrGazeSetConfigPath_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStartGaze", (PFN_xrVoidFunction*)&xrStartGaze_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStopGaze", (PFN_xrVoidFunction*)&xrStopGaze_Func));
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSetExposure", (PFN_xrVoidFunction*)&xrSetExposure_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSetBright", (PFN_xrVoidFunction*)&xrSetBright_Func));
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetGazeCallback", (PFN_xrVoidFunction*)&xrGetGazeCallback_Func));
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrChangeIpd", (PFN_xrVoidFunction*)&xrChangeIpd_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrCameraDistance", (PFN_xrVoidFunction*)&xrCameraDistance_Func));
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetDeviceCameraImageData", (PFN_xrVoidFunction*)&xrGetDeviceCameraImageData_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetDeviceCameraImageSize", (PFN_xrVoidFunction*)&xrGetDeviceCameraImageSize_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGazeCalibrationPubBegin", (PFN_xrVoidFunction*)&xrGazeCalibrationPubBegin_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGazeCalibrationPubSetConfGaze", (PFN_xrVoidFunction*)&xrGazeCalibrationPubSetConfGaze_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGazeCalibrationPubEnd", (PFN_xrVoidFunction*)&xrGazeCalibrationPubEnd_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSetGazeConfigs", (PFN_xrVoidFunction*)&xrSetGazeConfigs_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSetPref", (PFN_xrVoidFunction*)&xrSetPref_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetPref", (PFN_xrVoidFunction*)&xrGetPref_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetHiValue", (PFN_xrVoidFunction*)&xrGetHiValue_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetSrValue", (PFN_xrVoidFunction*)&xrGetSrValue_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGazeEnableDump", (PFN_xrVoidFunction*)&xrGazeEnableDump_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSetUsrEyeReady", (PFN_xrVoidFunction*)&xrSetUsrEyeReady_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetGazeIpd", (PFN_xrVoidFunction*)&xrGetGazeIpd_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStartEyetracking", (PFN_xrVoidFunction*)&xrStartEyetracking_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStopEyetracking", (PFN_xrVoidFunction*)&xrStopEyetracking_Func));
	
	return InNext;
}

const void* XvisioOpenXR::FGazeCalibrationPlugin::OnBeginSession(XrSession InSession, const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("FGazeCalibrationPlugin :  OnBeginSession"));
	Session = InSession;
	WorldToMetersScale = UHeadMountedDisplayFunctionLibrary::GetWorldToMetersScale(GWorld);
	return InNext;
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GazeSetConfigPath(const FString& InPath)
{
	if (!xrGazeSetConfigPath_Func || Session == XR_NULL_HANDLE)
		return false;
	
	if (InPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("jsonPath is empty"));
		return false;
	}
	
	
	FTCHARToUTF8 Utf8(*InPath);
	const char* Utf8Path = Utf8.Get();
	
	UE_LOG(LogTemp, Warning, TEXT("Gaze config path (UTF-8): %s"), UTF8_TO_TCHAR(Utf8Path));

	if (!Utf8Path)
	{
		UE_LOG(LogTemp, Error, TEXT("GazeSetConfigPath: UTF8 conversion failed"));
		return false;
	}
	
	
	XrResult Result = xrGazeSetConfigPath_Func(Session,  const_cast<unsigned char*>(reinterpret_cast<const unsigned char*>(Utf8.Get())) );
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGazeSetConfigPath_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::StartGaze(int32_t* ret)
{
	if (!xrStartGaze_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStartGaze_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStartGaze_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::StopGaze(int32_t* ret)
{
	if (!xrStopGaze_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStopGaze_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStopGaze_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::SetExposure(int32_t leftGain, float leftTimeMs, int32_t rightGain,
                                                       float rightTimeMs, int32_t* ret)
{
	if (!xrSetExposure_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrSetExposure_Func(Session,leftGain, leftTimeMs, rightGain, rightTimeMs, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSetExposure_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetGazeCallback(XrEtEyeDataEx* data)
{
	if (!xrGetGazeCallback_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetGazeCallback_Func(Session,data);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetGazeCallback_Func is OK"));
		return true;
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetGazeCallback_Func is false"));
		return false;
	}
}



bool XvisioOpenXR::FGazeCalibrationPlugin::SetBright(int32_t eye, int32_t led, int32_t brightness, int32_t* ret)
{
	if (!xrSetBright_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrSetBright_Func(Session,eye, led, brightness, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSetBright_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::ChangeIpd(float distance, int32_t* ret)
{
	if (!xrChangeIpd_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrChangeIpd_Func(Session, distance, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrChangeIpd_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::CameraDistance(uint32_t distance)
{
	if (!xrCameraDistance_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrCameraDistance_Func(Session,distance);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrCameraDistance_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetDeviceCameraImageData(unsigned char* data, uint32_t type, uint32_t width,
                                                                    uint32_t height, double* timestamp)
{
	if (!xrGetDeviceCameraImageData_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetDeviceCameraImageData_Func(Session,data, type, width, height, timestamp);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetDeviceCameraImageData_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetDeviceCameraImageSize(uint32_t type, uint32_t* width, uint32_t* height)
{
	if (!xrGetDeviceCameraImageSize_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetDeviceCameraImageSize_Func(Session, type, width, height);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetDeviceCameraImageSize_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GazeCalibrationPubBegin(int32_t ETIndex, int32_t* ret)
{
	if (!xrGazeCalibrationPubBegin_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGazeCalibrationPubBegin_Func(Session, ETIndex, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGazeCalibrationPubBegin_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GazeCalibrationPubSetConfGaze(int32_t type, int32_t ETIndex,float x, float y, uint32_t* ret)
{
	if (!xrGazeCalibrationPubSetConfGaze_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGazeCalibrationPubSetConfGaze_Func(Session,type, ETIndex, x,y,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGazeCalibrationPubSetConfGaze_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GazeCalibrationPubEnd(int32_t ETIndex, int32_t* ret)
{
	if (!xrGazeCalibrationPubEnd_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGazeCalibrationPubEnd_Func(Session,ETIndex, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGazeCalibrationPubEnd_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::SetGazeConfigs(xrGazeConfigsData* data, int32_t* ret)
{
	if (!xrSetGazeConfigs_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrSetGazeConfigs_Func(Session,data, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSetGazeConfigs_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::SetPref(int32_t etIdx, unsigned char* data, int32_t size, int32_t* ret)
{
	if (!xrSetPref_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrSetPref_Func(Session,etIdx, data, size, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSetPref_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetPref(int32_t etIdx, unsigned char* data, int32_t* size, int32_t* ret)
{
	if (!xrGetPref_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetPref_Func(Session,etIdx, data, size, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetPref_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetHiValue(int32_t* ret)
{
	if (!xrGetHiValue_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetHiValue_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetHiValue_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetSrValue(int32_t* ret)
{
	if (!xrGetSrValue_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetSrValue_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetSrValue_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GazeEnableDump(int32_t enable, int32_t* ret)
{
	if (!xrGazeEnableDump_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGazeEnableDump_Func(Session,enable, ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGazeEnableDump_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::SetUsrEyeReady(int32_t* ret)
{
	if (!xrSetUsrEyeReady_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrSetUsrEyeReady_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSetUsrEyeReady_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::GetGazeIpd(float_t* ipd)
{
	if (!xrGetGazeIpd_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetGazeIpd_Func(Session,ipd);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetGazeIpd_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::StartEyetracking(int32_t* ret)
{
	if (!xrStartEyetracking_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStartEyetracking_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStartEyetracking_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FGazeCalibrationPlugin::StopEyetracking(int32_t* ret)
{
	if (!xrStopEyetracking_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStopEyetracking_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStopEyetracking_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}
