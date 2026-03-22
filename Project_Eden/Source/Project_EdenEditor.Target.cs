// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Project_EdenEditorTarget : TargetRules
{
	public Project_EdenEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
<<<<<<< HEAD
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Project_Eden");
=======
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
        ExtraModuleNames.Add("Project_Eden");
>>>>>>> pcg-map
	}
}
