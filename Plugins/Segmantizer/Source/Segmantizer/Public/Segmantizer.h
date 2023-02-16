// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ClassManager.h"

class UClassMappingAsset;

class FSegmantizerModule : public IModuleInterface
{
public:
	FClassManager ClassManager;
	
	UPROPERTY()
	UClassMappingAsset* ClassDataAsset = nullptr;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void SetViewToSemantic();
	void SetViewToLit();

	void Save();
};
