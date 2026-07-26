// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class Project_EdenServerTarget : TargetRules
{
	public Project_EdenServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Project_Eden");
		// Keep the regular Windows-subsystem server executable for deployment,
		// and also build a console-subsystem sibling for local administration.
		// The -Cmd executable attaches directly to the launcher CMD so stdout
		// contains the live dedicated-server log.
		bBuildAdditionalConsoleApp = true;

		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			BuildEnvironment = TargetBuildEnvironment.Unique;
			bUseLoggingInShipping = true;
		}
	}
}
