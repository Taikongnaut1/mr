#include "Standalone/FoveatedReaderPlugin.h"

#include "GameDelegates.h"


XvisioOpenXR::FFoveatedReaderPlugin::FFoveatedReaderPlugin()
{
}

void XvisioOpenXR::FFoveatedReaderPlugin::Register()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().AddRaw(this, &FFoveatedReaderPlugin::OnEndPlay);
}

void XvisioOpenXR::FFoveatedReaderPlugin::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
	FGameDelegates::Get().GetEndPlayMapDelegate().RemoveAll(this);
}

void XvisioOpenXR::FFoveatedReaderPlugin::OnEndPlay()
{
}

bool XvisioOpenXR::FFoveatedReaderPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	UE_LOG(LogTemp, Log, TEXT("KeyEventPlugin :  GetRequiredExtensions"));
	OutExtensions.Add(XR_XVISIO_quad_views_PUB_EXTENSION_NAME);
	OutExtensions.Add(XR_XVISIO_foveate_rendering_PUB_EXTENSION_NAME);
	return true;
}

const void* XvisioOpenXR::FFoveatedReaderPlugin::OnCreateSession(XrInstance InInstance, XrSystemId InSystem,
	const void* InNext)
{
	return IOpenXRExtensionPlugin::OnCreateSession(InInstance, InSystem, InNext);
}

const void* XvisioOpenXR::FFoveatedReaderPlugin::OnBeginSession(XrSession InSession, const void* InNext)
{
	return IOpenXRExtensionPlugin::OnBeginSession(InSession, InNext);
}
