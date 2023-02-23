// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleSensorGridShadersModule.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "ShaderCore.h"

IMPLEMENT_MODULE(ICitySampleSensorGridShadersModule, CitySampleSensorGridShaders);

void ICitySampleSensorGridShadersModule::StartupModule()
{
	// Maps virtual shader source directory /Plugin/FX/Niagara to the plugin's actual Shaders directory.
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("CitySampleSensorGrid"))->GetBaseDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping(TEXT("/CitySampleSensorGrid"), PluginShaderDir);
}