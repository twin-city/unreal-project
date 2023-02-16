// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TwinCityEditorTarget : TargetRules
{
	public TwinCityEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("TwinCity");
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_1;
	}
}
