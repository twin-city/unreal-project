// Copyright Epic Games, Inc. All Rights Reserved.

namespace UnrealBuildTool.Rules
{
	public class CitySampleSensorGrid : ModuleRules
	{
		public CitySampleSensorGrid(ReadOnlyTargetRules Target) : base(Target)
		{
			// Include Renderer/Private to have access to default resources
			PrivateIncludePaths.AddRange(
				new string[] {
					ModuleDirectory + "/Private",
					EngineDirectory + "/Source/Runtime/Renderer/Private",
				});
			PublicIncludePaths.Add(ModuleDirectory + "/Public");

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"CitySampleSensorGridShaders",
					"Projects",
					"MeshDescription",
					"MovieScene",
					"NiagaraCore",
					"Niagara",
					"NiagaraShader",
					"RenderCore",
					"Renderer",
					"VectorVM",
					"RHI",
				});
		}
	}
}
