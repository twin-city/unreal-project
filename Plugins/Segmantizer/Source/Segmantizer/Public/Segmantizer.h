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
	
	class UClassMappingAsset* ClassDataAsset = nullptr;

	struct FCaptureRequest
	{
		FString				Filename;
		FDateTime			DateTime;
		FRequestDelegate	CaptureDelegate;
	};

	TQueue<FCaptureRequest> CaptureQueue;
	FRequestDelegate RequestDelegate;
	FDelegateHandle RequestDelegateHandle;

	void ShotCapture(const FString& Filename, const FDateTime& DateTime) const;
	bool UnrollQueue();

	bool LoadClassDataAsset(const FString& Filepath);
	void CreateClassDataAsset(const FString& Directory, const FString& Filename);
	
public:
	FClassManager ClassManager;

	UClassMappingAsset* GetClassDataAsset();
	
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void SetViewToSemantic();
	void SetViewToLit();

	void SaveClassDataAsset() const;

	void CaptureStart();
	
	void CaptureLoop();
	void CaptureEnd();

	static const FString ModuleDirectory;
};