using UnrealBuildTool;

public class CrossoverTarget : TargetRules
{
    public CrossoverTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("Crossover");
    }
}
