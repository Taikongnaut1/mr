#include "Standalone/FXvisioFunctionPlugin.h"

#include "GameDelegates.h"
#include "Standalone/OpenXRCommon.h"
#include "OpenXRCore.h"

XvisioOpenXR::FXvisioFunctionPlugin::FXvisioFunctionPlugin()
{
}

void XvisioOpenXR::FXvisioFunctionPlugin::Register()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FXvisioFunctionPlugin::OnEndPlay);
}

void XvisioOpenXR::FXvisioFunctionPlugin::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}

void XvisioOpenXR::FXvisioFunctionPlugin::OnEndPlay()
{

}

bool XvisioOpenXR::FXvisioFunctionPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	UE_LOG(LogTemp, Log, TEXT("FXvisioFunctionPlugin :  GetRequiredExtensions"));
	OutExtensions.Add(XR_XVISIO_GAZE_CALIBRATION_PUB_EXTENSION_NAME);
	return true;
}

const void* XvisioOpenXR::FXvisioFunctionPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem,
	const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  OnCreateSession"));

	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrDetectTags", (PFN_xrVoidFunction*)&xrDetectTags_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStopDetectTags", (PFN_xrVoidFunction*)&xrStopDetectTags_Func));

	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetCslamCallbackData", (PFN_xrVoidFunction*)&xrGetCslamCallbackData_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrLoadMapAndSwitchToCslamXVISIO", (PFN_xrVoidFunction*)&xrLoadMapAndSwitchToCslam_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrSaveMapAndSwitchToCslamXVISIO", (PFN_xrVoidFunction*)&xrSaveMapAndSwitchToCslam_Func));

	return InNext;
}

const void* XvisioOpenXR::FXvisioFunctionPlugin::OnBeginSession(XrSession InSession, const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  OnBeginSession"));
	Session = InSession;
	return InNext;
}

bool XvisioOpenXR::FXvisioFunctionPlugin::getApriltag(unsigned char* tagFamily, double size, XrAprilTagDataXV* tagsArray, uint32_t* tagSize, uint32_t arraySize, uint32_t type)
{
	if (!xrDetectTags_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrDetectTags_Func(Session,tagFamily,size,tagsArray,tagSize,arraySize,type);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrDetectTags_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FXvisioFunctionPlugin::StopApriltag(uint32_t ret)
{
	if (!xrStopDetectTags_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStopDetectTags_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStopDetectTags_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FXvisioFunctionPlugin::GetCslamCallbackData(XrCSlamCallbackDataXV* callbackData)
{
	if (!xrGetCslamCallbackData_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetCslamCallbackData_Func(Session, callbackData);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetCslamCallbackData_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FXvisioFunctionPlugin::LoadMapAndSwitchToCslam(const FString& mapPath)
{
	if (!xrLoadMapAndSwitchToCslam_Func || Session == XR_NULL_HANDLE)
		return false;

	FTCHARToUTF8 Utf8(*mapPath);
	XrResult Result = xrLoadMapAndSwitchToCslam_Func(Session, (char*)Utf8.Get());
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrLoadMapAndSwitchToCslam_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FXvisioFunctionPlugin::SaveMapAndSwitchToCslam(const FString& mapPath)
{
	if (!xrSaveMapAndSwitchToCslam_Func || Session == XR_NULL_HANDLE)
		return false;

	FTCHARToUTF8 Utf8(*mapPath);
	XrResult Result = xrSaveMapAndSwitchToCslam_Func(Session, (char*)Utf8.Get());
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrSaveMapAndSwitchToCslam_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}