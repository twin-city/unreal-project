// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"
#include "ClassManager.h"

UENUM()
enum EViewType
{
	LIT,
	SEMANTIC,
	NONE
};


class FSegmantizerModule : public IModuleInterface
{
	DECLARE_DELEGATE(FRequestDelegate);
	
	struct FCaptureRequest
	{
		FString				Filename;
		FRequestDelegate	CaptureDelegate;
	};
	
	TQueue<FCaptureRequest> CaptureQueue;
	FTickerDelegate TickDelegate;
	FTSTicker::FDelegateHandle TickDelegateHandle;

	void ShotCapture(const FString& Filename);
	
public:
	FClassManager ClassManager;
	
	class UClassMappingAsset* ClassDataAsset = nullptr;
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void SetViewToSemantic();
	void SetViewToLit();

	void Save();

	void CaptureStart();
	
	bool CaptureLoop(float DeltaTime);
	void CaptureEnd();
};
