#include "Standalone/HandJointExtraPlugin.h"
#include "OpenXRCore.h"
#include "IXRTrackingSystem.h"
#include "Engine/Engine.h"

namespace XvisioOpenXR
{

void FHandJointExtraPlugin::Register()
{
	IModularFeatures::Get().RegisterModularFeature(GetModularFeatureName(), this);
}

void FHandJointExtraPlugin::Unregister()
{
	IModularFeatures::Get().UnregisterModularFeature(GetModularFeatureName(), this);
}

bool FHandJointExtraPlugin::GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions)
{
	TArray<XrApiLayerProperties> Properties;
	uint32_t Count = 0;
	XR_ENSURE(xrEnumerateApiLayerProperties(0, &Count, nullptr));
	Properties.SetNum(Count);
	for (auto& Prop : Properties)
	{
		Prop = XrApiLayerProperties{ XR_TYPE_API_LAYER_PROPERTIES };
	}
	XR_ENSURE(xrEnumerateApiLayerProperties(Count, &Count, Properties.GetData()));

	for (const XrApiLayerProperties& Layer : Properties)
	{
		if (FCStringAnsi::Strstr(Layer.layerName, "XR_APILAYER_VIVE_hand_tracking") &&
			Layer.layerVersion <= 1)
		{
			return false;
		}
	}

	OutExtensions.Add("XR_EXT_hand_tracking");
	return true;
}

const void* FHandJointExtraPlugin::OnGetSystem(XrInstance InInstance, const void* InNext)
{
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrCreateHandTrackerEXT", (PFN_xrVoidFunction*)&xrCreateHandTracker));
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrDestroyHandTrackerEXT", (PFN_xrVoidFunction*)&xrDestroyHandTracker));
	XR_ENSURE(xrGetInstanceProcAddr(InInstance, "xrLocateHandJointsEXT", (PFN_xrVoidFunction*)&xrLocateHandJoints));

	return InNext;
}

const void* FHandJointExtraPlugin::OnBeginSession(XrSession InSession, const void* InNext)
{
	Session = InSession;

	static FName SystemName(TEXT("OpenXR"));
	if (GEngine->XRSystem.IsValid() && (GEngine->XRSystem->GetSystemName() == SystemName))
	{
		XRTrackingSystem = GEngine->XRSystem.Get();
	}

	FHandState& LeftHandState = GetLeftHandState();
	{
		XrHandTrackerCreateInfoEXT CreateInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		CreateInfo.hand = XR_HAND_LEFT_EXT;
		CreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		XR_ENSURE(xrCreateHandTracker(InSession, &CreateInfo, &LeftHandState.HandTracker));
	}

	FHandState& RightHandState = GetRightHandState();
	{
		XrHandTrackerCreateInfoEXT CreateInfo{ XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT };
		CreateInfo.hand = XR_HAND_RIGHT_EXT;
		CreateInfo.handJointSet = XR_HAND_JOINT_SET_DEFAULT_EXT;
		XR_ENSURE(xrCreateHandTracker(InSession, &CreateInfo, &RightHandState.HandTracker));
	}

	return InNext;
}

void FHandJointExtraPlugin::OnDestroySession(XrSession InSession)
{
	if (GetLeftHandState().HandTracker)
	{
		xrDestroyHandTracker(GetLeftHandState().HandTracker);
		GetLeftHandState().HandTracker = XR_NULL_HANDLE;
	}
	if (GetRightHandState().HandTracker)
	{
		xrDestroyHandTracker(GetRightHandState().HandTracker);
		GetRightHandState().HandTracker = XR_NULL_HANDLE;
	}
	Session = XR_NULL_HANDLE;
}

void FHandJointExtraPlugin::UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace)
{
	if (!Session || !xrLocateHandJoints || !XRTrackingSystem)
	{
		return;
	}

	const float WorldToMetersScale = XRTrackingSystem->GetWorldToMetersScale();

	for (int i = 0; i < 2; ++i)
	{
		FHandState& HandState = HandStates[i];

		XrResult result = xrLocateHandJoints(HandState.HandTracker, TrackingSpace, DisplayTime, &HandState.Locations);
		HandState.ReceivedJointPoses = XR_SUCCEEDED(result);

		if (HandState.ReceivedJointPoses)
		{
			for (int j = 0; j < XR_HAND_JOINT_COUNT_EXT; ++j)
			{
				const XrPosef& Pose = HandState.JointLocations[j].pose;
				HandState.KeypointTransforms[j] = ToFTransform(Pose, WorldToMetersScale);
				HandState.Radii[j] = HandState.JointLocations[j].radius * WorldToMetersScale;
			}
		}
	}
}

FHandJointExtraPlugin::FHandState::FHandState()
{
	FMemory::Memzero(JointLocations, sizeof(JointLocations));

	Locations.jointCount = XR_HAND_JOINT_COUNT_EXT;
	Locations.jointLocations = JointLocations;
}

FHandJointExtraPlugin::FHandState& FHandJointExtraPlugin::GetLeftHandState()
{
	return HandStates[0];
}

FHandJointExtraPlugin::FHandState& FHandJointExtraPlugin::GetRightHandState()
{
	return HandStates[1];
}

const FHandJointExtraPlugin::FHandState& FHandJointExtraPlugin::GetLeftHandState() const
{
	return HandStates[0];
}

const FHandJointExtraPlugin::FHandState& FHandJointExtraPlugin::GetRightHandState() const
{
	return HandStates[1];
}

bool FHandJointExtraPlugin::GetHandJointExtraData(EControllerHand Hand,
	TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations, TArray<float>& OutRadii)
{
	if (Hand != EControllerHand::Left && Hand != EControllerHand::Right)
	{
		return false;
	}

	const FHandState& HandState = (Hand == EControllerHand::Left) ? GetLeftHandState() : GetRightHandState();

	if (!HandState.ReceivedJointPoses)
	{
		return false;
	}

	OutPositions.SetNum(EHandKeypointCount);
	OutRotations.SetNum(EHandKeypointCount);
	OutRadii.SetNum(EHandKeypointCount);

	const FTransform& TrackingToWorldTransform = XRTrackingSystem->GetTrackingToWorldTransform();
	for (int i = 0; i < EHandKeypointCount; ++i)
	{
		FTransform WorldTransform = HandState.KeypointTransforms[i] * TrackingToWorldTransform;
		OutPositions[i] = WorldTransform.GetLocation();
		OutRotations[i] = WorldTransform.GetRotation();
		OutRadii[i] = HandState.Radii[i];
	}

	return true;
}

} // namespace XvisioOpenXR