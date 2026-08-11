#pragma once
#include <openxr/openxr.h>

#define XR_XV_DETECT_COUNT                64

#define XR_XVisio_gaze_calibration_pub 1
#define XR_XVisio_gaze_calibration_pub_SPEC_VERSION 1
#define XR_XVISIO_GAZE_CALIBRATION_PUB_EXTENSION_NAME "XR_EXT_api"

#define XR_XVISIO_foveate_rendering_PUB_EXTENSION_NAME "XR_VARJO_foveated_rendering"
#define XR_XVISIO_quad_views_PUB_EXTENSION_NAME "XR_VARJO_quad_views"

typedef struct xrGazeConfigsData {
	int32_t width;
	int32_t height;
	int32_t ipdDist;
	int32_t srValue;
	int32_t etWidth;
	int32_t etHeight;
	int32_t loplength;
	float etFoclen;
	float etOccupy;
	float ftFoclen;
} xrGazeConfigsData;

typedef struct XrGazePointXVISIO {
	uint32_t      gazeBitMask;
	XrVector3f    gazePoint;
	XrVector3f    rawPoint;
	XrVector3f    smoothPoint;
	XrVector3f    gazeOrigin;
	XrVector3f    gazeDirection;
	float         re;
	uint32_t      exDataBitMask;
	float         confidence;
	int32_t       acceptedStat;
} XrGazePointXVISIO;

typedef struct XrGazePupilINFOXVISIO {
	uint32_t      pupilBitMask;
	XrVector2f    pupilCenter;
	float         pupilDistance;
	float         pupilDiameter;
	float         pupilDiameterMM;
	float         pupilMinorAxis;
	float         pupilMinorAxisMM;
} XrGazePupilINFOXVISIO;

typedef struct XrGazeEyeExDataXVISIO {
	uint32_t    eyeDataExBitMask;
	int32_t     blink;
	float       openness;
	float       eyelidUp;
	float       eyelidDown;
} XrGazeEyeExDataXVISIO;

typedef struct XrEtEyeDataEx {
	unsigned long long    timestamp;
	int32_t               recommend;
	XrGazePointXVISIO     recomGaze;
	XrGazePointXVISIO     leftGaze;
	XrGazePointXVISIO     rightGaze;
	XrGazePupilINFOXVISIO     leftPupil;
	XrGazePupilINFOXVISIO     rightPupil;
	XrGazeEyeExDataXVISIO     leftExData;
	XrGazeEyeExDataXVISIO     rightExData;
	float                 ipd;
	int32_t               leftEyeMove;
	int32_t               rightEyeMove;
} XrEtEyeDataEx;

typedef struct XrEventData{
	double hostTimestamp;
	int64_t degeTimestampUs;
	int32_t eventType;
	int32_t eventState;
	
}XrEventData;

typedef struct XrTagDataXV {
	uint32_t      tagID;
	XrVector3f    position;
	XrVector3f    orientation;
	XrVector4f    quaternion;
	uint32_t      edgeTimestamp;
	double        hostTimestamp;
	float         confidence;
} XrTagDataXV;

typedef struct XrAprilTagDataXV {
	XrTagDataXV    detect[XR_XV_DETECT_COUNT];
} XrAprilTagDataXV;


typedef XrResult (XRAPI_PTR *PFN_xrGazeSetConfigPath)(XrSession session, unsigned char* path);
typedef XrResult (XRAPI_PTR *PFN_xrStartGaze)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrStopGaze)(XrSession session, int32_t* ret);

typedef XrResult (XRAPI_PTR *PFN_xrSetExposure)(XrSession session, int32_t leftGain, float leftTimeMs, int32_t rightGain, float rightTimeMs, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrSetBright)(XrSession session, int32_t eye,int32_t led,int32_t brightness,int32_t* ret);

typedef XrResult (XRAPI_PTR *PFN_xrGetGazeCallback)(XrSession session, XrEtEyeDataEx* data);

typedef XrResult (XRAPI_PTR *PFN_xrChangeIpd)(XrSession session, float distance,int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrCameraDistance)(XrSession session, uint32_t distance);

typedef XrResult (XRAPI_PTR *PFN_xrGetDeviceCameraImageData)(XrSession session, unsigned char* data,uint32_t type, uint32_t width, uint32_t height,double* timestamp);
typedef XrResult (XRAPI_PTR *PFN_xrGetDeviceCameraImageSize)(XrSession session, uint32_t type, uint32_t* width, uint32_t* height);
typedef XrResult (XRAPI_PTR *PFN_xrGazeCalibrationPubBegin)(XrSession session, int32_t ETIndex, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGazeCalibrationPubSetConfGaze)(XrSession session, int32_t type, uint32_t ETIndex,float x,float y,uint32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGazeCalibrationPubEnd)(XrSession session, int32_t ETIndex, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrSetGazeConfigs)(XrSession session, xrGazeConfigsData* data, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrSetPref)(XrSession session, int32_t etIdx, unsigned char* data, int32_t size, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGetPref)(XrSession session, int32_t etIdx, unsigned char* data, int32_t* size, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGetHiValue)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGetSrValue)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGazeEnableDump)(XrSession session, int32_t enable, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrSetUsrEyeReady)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGetGazeIpd)(XrSession session, float_t* ipd);
typedef XrResult (XRAPI_PTR *PFN_xrStartEyetracking)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrStopEyetracking)(XrSession session, int32_t* ret);

typedef XrResult (XRAPI_PTR *PFN_xrStopDetectTags)(XrSession session, uint32_t type);
typedef XrResult (XRAPI_PTR *PFN_xrDetectTags)(XrSession session, unsigned char* tagFamily, double size, XrAprilTagDataXV* tagsArray, uint32_t* tagSize, uint32_t arraySize, uint32_t type);


typedef struct XrCSlamCallbackDataXV {
	uint32_t    map_quality;
	float       percent;
	uint32_t    status_of_saved_map;
} XrCSlamCallbackDataXV;

typedef XrResult (XRAPI_PTR *PFN_xrGetCslamCallbackData)(XrSession session, XrCSlamCallbackDataXV* callbackData);
typedef XrResult (XRAPI_PTR *PFN_xrLoadMapAndSwitchToCslamXVISIO)(XrSession session, char* mapPath);
typedef XrResult (XRAPI_PTR *PFN_xrSaveMapAndSwitchToCslamXVISIO)(XrSession session, char* mapPath);

typedef XrResult (XRAPI_PTR *PFN_xrStartEvent)(XrSession session, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrGetEventData)(XrSession session, XrEventData* eventData, int32_t* ret);
typedef XrResult (XRAPI_PTR *PFN_xrStopEvent)(XrSession session, int32_t* ret);
//typedef XrResult (XRAPI_PTR *PFN_xrEnumerateViewConfigurations)(XrSession session, XrEventData* eventData, int32_t* ret);

typedef XrResult (XRAPI_PTR *PFN_xrDeviceRgbSetBrightnessXVISIO)(XrSession session, uint32_t brightness);
typedef XrResult (XRAPI_PTR *PFN_xrTywSetBrightnessXVISIO)(XrSession session, uint32_t brightness);

typedef XrResult (XRAPI_PTR *PFN_xrCreateHandTracker)(XrSession session, const XrHandTrackerCreateInfoEXT* createInfo, XrHandTrackerEXT* handTracker);
typedef XrResult (XRAPI_PTR *PFN_xrDestroyHandTracker)(XrHandTrackerEXT handTracker);
typedef XrResult (XRAPI_PTR *PFN_xrLocateHandJoints)(XrHandTrackerEXT handTracker, XrSpace baseSpace, XrTime time, XrHandJointLocationsEXT* locations);


