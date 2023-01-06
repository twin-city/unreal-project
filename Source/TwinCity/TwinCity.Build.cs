// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TwinCity : ModuleRules
{
	public TwinCity(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "ZoneGraph" });
	}
}
