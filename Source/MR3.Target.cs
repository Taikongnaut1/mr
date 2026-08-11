using UnrealBuildTool;
using System.Collections.Generic;

public class MR3Target : TargetRules
{
    public MR3Target(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
        ExtraModuleNames.Add("MR3");
    }
}
