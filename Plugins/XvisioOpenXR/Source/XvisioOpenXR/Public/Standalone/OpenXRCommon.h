// Copyright (c) 2020 Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <openxr/openxr.h>


namespace XvisioOpenXR
{
#if DO_CHECK
	// Modified from OpenXRCore's XR_ENSURE which uses engine OpenXR headers to get a result string.
	// Otherwise, this will fail to build when using newer OpenXR headers in this plugin.
#define XR_ENSURE_MSFT(x) [] (XrResult Result) \
{ \
return ensureMsgf(XR_SUCCEEDED(Result), TEXT("OpenXR call failed with result: %d"), Result); \
} (x)
#else
#define XR_ENSURE_MSFT(x) XR_SUCCEEDED(x)
#endif

	XrPath GetXrPath(XrInstance Instance, const char* PathString);
}
