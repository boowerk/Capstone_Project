// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Project_Eden : ModuleRules
{
	public Project_Eden(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", "CoreUObject",
            "Engine", 
			"HTTP", 
			"InputCore", 
			"EnhancedInput",
			"UMG",
			"Json",
			"JsonUtilities",
            "PCG",
            "GameplayAbilities",
            "GameplayTasks",
            "GameplayTags",
            "AIModule",
            "NavigationSystem",
        });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
        
    }
}
