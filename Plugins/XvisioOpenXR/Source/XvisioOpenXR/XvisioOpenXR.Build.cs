

using UnrealBuildTool;
using System;
using System.IO;

public class XvisioOpenXR : ModuleRules
{
	public XvisioOpenXR(ReadOnlyTargetRules Target) : base(Target)
	{
		System.Console.WriteLine("=== [XvisioOpenXR] Start Build.cs ===");

		PCHUsage = PCHUsageMode.NoPCHs;

		PrivateIncludePaths.AddRange(new string[] {
			Path.Combine("XvisioOpenXR", "Private"),
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"DeveloperSettings",
			"InputDevice",
			"OpenXRHMD",
			"XRBase",
			"HeadMountedDisplay",
			"Json",
			"EnhancedInput"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"OpenXRAR",
			"OpenXRInput",
			//
		});

		AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenXR");

		string ModuleRoot = ModuleDirectory; 
		string ThirdPartyRoot = Path.GetFullPath(Path.Combine(ModuleRoot, "ThirdParty/"));
		string BuildConfig = Target.Configuration.ToString();

		//----------------------------------------------------------
		// 🟩 获取项目根目录
		//----------------------------------------------------------
		string ProjectRoot = null;
		try
		{
			if (Target.ProjectFile != null)
			{
				ProjectRoot = Path.GetDirectoryName(Target.ProjectFile.FullName);
			}
			else
			{
				// 没有绑定具体 .uproject 文件时（比如编译插件或引擎模块）
				ProjectRoot = Path.GetFullPath(Path.Combine(ModuleRoot, "../../../../"));
			}
		}
		catch
		{
			ProjectRoot = Path.GetFullPath(Path.Combine(ModuleRoot, "../../../../"));
		}

		System.Console.WriteLine("[XvisioOpenXR] Detected Project Root: " + ProjectRoot);

		//----------------------------------------------------------
		// 🟩 Windows 平台
		//----------------------------------------------------------
		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			string Win64ThirdPartyDir = Path.Combine(ThirdPartyRoot, "Win64");
			string DllName = "xv_client.dll";
			string LibName = "xv_client.lib";

			string DllSourcePath = Path.Combine(Win64ThirdPartyDir, DllName);
			string LibSourcePath = Path.Combine(Win64ThirdPartyDir, LibName);

			string ProjectDllPath = Path.Combine(ProjectRoot, "Binaries/Win64", DllName);
			string RuntimeDllTarget = "$(ProjectDir)/Binaries/Win64/" + DllName;
			if (File.Exists(DllSourcePath) && File.Exists(LibSourcePath))
			{
				PublicAdditionalLibraries.Add(LibSourcePath);
				PublicDelayLoadDLLs.Add(DllName);
				RuntimeDependencies.Add(RuntimeDllTarget, DllSourcePath);

				try
				{
					Directory.CreateDirectory(Path.GetDirectoryName(ProjectDllPath));
					File.Copy(DllSourcePath, ProjectDllPath, true);
					System.Console.WriteLine("[XvisioOpenXR] ✅ Copied DLL to " + ProjectDllPath);
				}
				catch (Exception e)
				{
					System.Console.WriteLine("[XvisioOpenXR] ⚠ DLL copy failed: " + e.Message);
				}
			}
			else
			{
				System.Console.WriteLine("[XvisioOpenXR] ⚠ Missing xv_client.dll or xv_client.lib");
				System.Console.WriteLine("    DLL: " + DllSourcePath);
				System.Console.WriteLine("    LIB: " + LibSourcePath);
			}
		}

		//----------------------------------------------------------
		// 🟩 Linux 平台
		//----------------------------------------------------------
		else if (Target.Platform == UnrealTargetPlatform.Linux)
		{
			string linuxThirdPartyDir = Path.Combine(ThirdPartyRoot, "Linux");
			string soName = "libxv_client.so";
			string soSourcePath = Path.Combine(linuxThirdPartyDir, soName);

			string ProjectSoPath = Path.Combine(ProjectRoot, "Binaries/Linux", soName);
			string RuntimeSoTarget = "$(ProjectDir)/Binaries/Linux/" + soName;

			if (File.Exists(soSourcePath))
			{
				RuntimeDependencies.Add(RuntimeSoTarget, soSourcePath);

				try
				{
					Directory.CreateDirectory(Path.GetDirectoryName(ProjectSoPath));
					File.Copy(soSourcePath, ProjectSoPath, true);
					System.Console.WriteLine("[XvisioOpenXR] ✅ Copied SO to " + ProjectSoPath);
				}
				catch (Exception e)
				{
					System.Console.WriteLine("[XvisioOpenXR] ⚠ SO copy failed: " + e.Message);
				}
			}
			else
			{
				System.Console.WriteLine("[XvisioOpenXR] ⚠ Missing libxv_client.so");
				System.Console.WriteLine("    Path: " + soSourcePath);
			}
		}

		//----------------------------------------------------------
		// 🟩 Android 平台
		//----------------------------------------------------------
		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			PublicDefinitions.Add("XR_USE_PLATFORM_ANDROID=1");

			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"ApplicationCore",
				"Launch",
			});

			string ModulePath = Utils.MakePathRelativeTo(ModuleDirectory, Target.RelativeEnginePath);
			AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModulePath, "XvisioOpenXR_UPL.xml"));
		}
	}
}
