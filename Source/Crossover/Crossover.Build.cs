using UnrealBuildTool;

public class Crossover : ModuleRules
{
    public Crossover(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "EngineSettings", "InputCore", "EnhancedInput" });
        PrivateDependencyModuleNames.Add("SlateCore");
    }
}
