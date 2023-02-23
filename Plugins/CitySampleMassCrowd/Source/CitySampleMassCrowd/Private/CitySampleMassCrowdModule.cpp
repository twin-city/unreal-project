// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "ICitySampleMassCrowdModule.h"

class FCitySampleMassCrowdModule : public ICitySampleMassCrowdModule
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FCitySampleMassCrowdModule, CitySampleMassCrowd)

void FCitySampleMassCrowdModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
}


void FCitySampleMassCrowdModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

