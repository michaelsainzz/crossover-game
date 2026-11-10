using UnrealBuildTool;

public class CrossoverEditorTarget : TargetRules
{
    public CrossoverEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Crossover");
    }
}
