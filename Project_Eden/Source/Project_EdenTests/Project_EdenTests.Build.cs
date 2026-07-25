using UnrealBuildTool;

public class Project_EdenTests : ModuleRules
{
	public Project_EdenTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"Core",
			"CoreUObject",
			"Engine",
			"NavigationSystem",
			"Niagara",
			"Project_Eden",
			"UMG",
		});
	}
}
