/*
 * @author: eddy
 * @Date: 2023-04-28 13:44:17
 * @LastEditTime: 2024-12-04 18:04:15
 * @LastEditors: eddy
 * @brief:
 * @copyright: Xvisio Tec
 * @FilePath: \undefinede:\Unreal Projects\mrtktest1101\ue4mrtk\Plugins\XvisioOpenXR\Source\XvisioOpenXR\Public\XvisioOpenXR.h
 *   当前文件
 */
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Standalone/FXvisioFunctionPlugin.h"
#include "Standalone/GazeCalibrationPlugin.h"
#include "Standalone/HandJointExtraPlugin.h"
#include "Standalone/KeyEventPlugin.h"

#if PLATFORM_ANDROID
#include "Android/AndroidPlatformMisc.h"
#include <android_native_app_glue.h>
// #include "JNI/RuntimeCheckerWrapper.h"
// #include "Misc/MessageDialog.h"
#include <dlfcn.h>
#endif // PLATFORM_ANDROID

#include "Modules/ModuleManager.h"

#include "CoreMinimal.h"

#include "Modules/ModuleInterface.h"

#include "../ExtensionsHead/XvisioOpenXRExtensions.h"

// #include "OpenXRHandTracking.h"
#include "IHeadMountedDisplay.h"
#include "Kismet/BlueprintFunctionLibrary.h"




DEFINE_LOG_CATEGORY_STATIC(LogXvisioOpenXRPlugin, Log, All);

#include "XvisioOpenXR.generated.h"

namespace XvisioOpenXR
{

	class XVISIOOPENXR_API FXVisioOpenXRModule : public IModuleInterface
	{
	public:
		/** 获取模块单例（UE 标准） */
		static FXVisioOpenXRModule* Get();

		/** 模块是否已加载 */
		static bool IsAvailable();

	public:
		virtual void StartupModule() override;
		virtual void ShutdownModule() override;

		/** 对外暴露插件接口 */
		FGazeCalibrationPlugin& GetGazeCalibrationPlugin()
		{
			return GazeCalibrationPlugin;
		}
		
		FKeyEventPlugin& GetKeyEventPlugin()
		{
			return KeyEventPlugin;
		}

		FXvisioFunctionPlugin& GetXvisioFunctionPlugin()
		{
			return XvisioFunctionPlugin;
		}

		FHandJointExtraPlugin& GetHandJointExtraPlugin()
		{
			return HandJointExtraPlugin;
		}

	private:
		FGazeCalibrationPlugin GazeCalibrationPlugin;
		FHandJointExtraPlugin HandJointExtraPlugin;
		FKeyEventPlugin KeyEventPlugin;

		FXvisioFunctionPlugin XvisioFunctionPlugin;
	};

} // namespace XvisioOpenXR



UCLASS(BlueprintType, Category = "XvisioOpenXR")
class XVISIOOPENXR_API UXvisioOpenXR : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="XvisioOpenXR|OpenXR", meta=(WorldContext="WorldContextObject"))
	static bool StartEvent(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="XvisioOpenXR|OpenXR", meta=(WorldContext="WorldContextObject"))
	static bool StopEvent(UObject* WorldContextObject);

	UFUNCTION(BlueprintCallable, Category="XvisioOpenXR|OpenXR")
	static bool GetHandJointExtraData(EControllerHand Hand,
		TArray<FVector>& OutPositions, TArray<FQuat>& OutRotations, TArray<float>& OutRadii);
};
