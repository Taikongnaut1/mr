using UnrealBuildTool;
using System.Collections.Generic;

public class MR3EditorTarget : TargetRules
{
    public MR3EditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("MR3");
    }
}
