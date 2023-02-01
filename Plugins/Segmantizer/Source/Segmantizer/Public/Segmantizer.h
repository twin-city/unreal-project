// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class UClassManager;
class UClassMappingAsset;

class FSegmantizerModule : public IModuleInterface
{
public:
	UPROPERTY()
	UClassManager* ClassManager = nullptr;
	
	UPROPERTY()
	UClassMappingAsset* ClassDataAsset = nullptr;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void SetViewToSemantic();

	void Save();
};
