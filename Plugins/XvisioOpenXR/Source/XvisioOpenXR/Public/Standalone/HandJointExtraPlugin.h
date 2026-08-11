#pragma once

#include "IOpenXRExtensionPlugin.h"
#include "IHandTracker.h"
#include "IXRTrackingSystem.h"
#include "../ExtensionsHead/XvisioOpenXRExtensions.h"
#include "Engine/Engine.h"

namespace XvisioOpenXR
{
	class XVISIOOPENXR_API FHandJointExtraPlugin : public IOpenXRExtensionPlugin
	{
	public:
		void Register();
		void Unregister();

		virtual bool GetRequiredExtensions(TArray<const ANSICHAR*>& OutExtensions) override;
		virtual const void* OnGetSystem(XrInstance InInstance, const void* InNext) override;
		virtual const void* OnBeginSession(XrSession InSession, const void* InNext) override;
		virtual void OnDestroySession(XrSession InSession) override;
		virtual void UpdateDeviceLocations(XrSession InSession, XrTime DisplayTime, XrSpace TrackingSpace) override;

		bool GetHandJointExtraData(EControllerHand Hand,
			TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations, TArray<float>& OutRadii);

	private:
		struct FHandState
		{
			FHandState();

			XrHandTrackerEXT HandTracker = XR_NULL_HANDLE;
			XrHandJointLocationEXT JointLocations[XR_HAND_JOINT_COUNT_EXT];
			XrHandJointLocationsEXT Locations{ XR_TYPE_HAND_JOINT_LOCATIONS_EXT };
			FTransform KeypointTransforms[EHandKeypointCount];
			float Radii[EHandKeypointCount];
			bool ReceivedJointPoses = false;
		};

		FHandState& GetLeftHandState();
		FHandState& GetRightHandState();
		const FHandState& GetLeftHandState() const;
		const FHandState& GetRightHandState() const;

		XrSession Session = XR_NULL_HANDLE;

		PFN_xrCreateHandTracker xrCreateHandTracker = nullptr;
		PFN_xrDestroyHandTracker xrDestroyHandTracker = nullptr;
		PFN_xrLocateHandJoints xrLocateHandJoints = nullptr;

		IXRTrackingSystem* XRTrackingSystem = nullptr;

		FHandState HandStates[2];
	};
}