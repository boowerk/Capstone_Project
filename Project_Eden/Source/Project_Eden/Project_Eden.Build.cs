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
			"Json",
			"JsonUtilities",
            "PCG",
			"AIModule",
            "StateTreeModule",
            "GameplayStateTreeModule",
            "UMG",
            "Slate"
        });

		PrivateDependencyModuleNames.AddRange(new string[] {});

        PublicIncludePaths.AddRange(new string[] {
            ModuleDirectory,
            "Project_Eden/TP_ThirdPerson",
            "Project_Eden/TP_ThirdPerson/Variant_Platforming",
            "Project_Eden/TP_ThirdPerson/Variant_Platforming/Animation",
            "Project_Eden/TP_ThirdPerson/Variant_Combat",
            "Project_Eden/TP_ThirdPerson/Variant_Combat/AI",
            "Project_Eden/TP_ThirdPerson/Variant_Combat/Animation",
            "Project_Eden/TP_ThirdPerson/Variant_Combat/Gameplay",
            "Project_Eden/TP_ThirdPerson/Variant_Combat/Interfaces",
            "Project_Eden/TP_ThirdPerson/Variant_Combat/UI",
            "Project_Eden/TP_ThirdPerson/Variant_SideScrolling",
            "Project_Eden/TP_ThirdPerson/Variant_SideScrolling/AI",
            "Project_Eden/TP_ThirdPerson/Variant_SideScrolling/Gameplay",
            "Project_Eden/TP_ThirdPerson/Variant_SideScrolling/Interfaces",
            "Project_Eden/TP_ThirdPerson/Variant_SideScrolling/UI"
        });

        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}
