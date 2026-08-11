#include "Standalone/KeyEventPlugin.h"

#include "GameDelegates.h"
#include "Standalone/OpenXRCommon.h"
#include "OpenXRCore.h"


XvisioOpenXR::FKeyEventPlugin::FKeyEventPlugin()
{
	Session = nullptr;
	xrGetEventData_Func = nullptr;
}

void XvisioOpenXR::FKeyEventPlugin::Register()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FKeyEventPlugin::OnEndPlay);
}

void XvisioOpenXR::FKeyEventPlugin::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}

void XvisioOpenXR::FKeyEventPlugin::OnEndPlay()
{
}

bool XvisioOpenXR::FKeyEventPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  GetRequiredExtensions"));
	OutExtensions.Add(XR_XVISIO_GAZE_CALIBRATION_PUB_EXTENSION_NAME);
	return true;
}

const void* XvisioOpenXR::FKeyEventPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem,
	const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  OnCreateSession"));
	
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStartEvent", (PFN_xrVoidFunction*)&xrStartEvent_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrGetEventData", (PFN_xrVoidFunction*)&xrGetEventData_Func));
	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrStopEvent", (PFN_xrVoidFunction*)&xrStopEvent_Func));

	XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrDeviceRgbSetBrightnessXVISIO", (PFN_xrVoidFunction*)&xrDeviceRgbSetBrightnessXVISIO_Func));
	//XR_ENSURE_MSFT(xrGetInstanceProcAddr(InInstance, "xrTywSetBrightnessXVISIO", (PFN_xrVoidFunction*)&xrTywSetBrightnessXVISIO_Func));
	
	return InNext;
}

const void* XvisioOpenXR::FKeyEventPlugin::OnBeginSession(XrSession InSession, const void* InNext)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  OnBeginSession"));
	Session = InSession;
	return InNext;
}

bool XvisioOpenXR::FKeyEventPlugin::StartEvent(int32_t* ret)
{
	UE_LOG(LogTemp, Log, TEXT("xrStartEvent_Func  ----------  1"));
	if (!xrStartEvent_Func || Session == XR_NULL_HANDLE)
		return false;
	UE_LOG(LogTemp, Log, TEXT("xrStartEvent_Func  ----------  2"));
	XrResult Result = xrStartEvent_Func(Session,ret);
	UE_LOG(LogTemp, Log, TEXT("xrStartEvent_Func  ----------  3 ____%d"),Result);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStartEvent_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FKeyEventPlugin::GetEventData(XrEventData* eventData, int32_t* ret)
{
	if (!xrGetEventData_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrGetEventData_Func(Session,eventData,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrGetEventData_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FKeyEventPlugin::StopEvent(int32_t* ret)
{
	if (!xrStopEvent_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrStopEvent_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrStopEvent_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}

bool XvisioOpenXR::FKeyEventPlugin::DeviceRgbSetBrightnessXVISIO(uint32_t ret)
{
	if (!xrDeviceRgbSetBrightnessXVISIO_Func || Session == XR_NULL_HANDLE)
		return false;
	XrResult Result = xrDeviceRgbSetBrightnessXVISIO_Func(Session,ret);
	if (XR_SUCCEEDED(Result))
	{
		UE_LOG(LogTemp, Log, TEXT("xrDeviceRgbSetBrightnessXVISIO_Func is OK"));
		return true;
	}
	else
	{
		return false;
	}
}
