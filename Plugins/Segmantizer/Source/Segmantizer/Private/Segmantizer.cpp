// Copyright Epic Games, Inc. All Rights Reserved.

#include "Segmantizer.h"

#include "ClassManager.h"
#include "ClassMappingAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/AssetManager.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SceneViewport.h"
#include "SLevelViewport.h"
#include "HighResScreenshot.h"

#define LOCTEXT_NAMESPACE "FSegmantizerModule"

const FString FSegmantizerModule::ModuleDirectory = TEXT("/Segmantizer");

void FSegmantizerModule::StartupModule()
{
	RequestDelegate = FRequestDelegate::CreateRaw(this, &FSegmantizerModule::CaptureLoop);

	// Register the plugin directory with the editor
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AddPath(*ModuleDirectory);
}

bool FSegmantizerModule::LoadClassDataAsset(const FString& Filepath)
{
	UObject* LoadedAsset = UAssetManager::GetStreamableManager().LoadSynchronous(Filepath);

	ClassDataAsset = Cast<UClassMappingAsset>(LoadedAsset);
	return ClassDataAsset != nullptr;
}

void FSegmantizerModule::CreateClassDataAsset(const FString& Directory, const FString& Filename)
{
	const FString Filepath = Directory / Filename;
	
	// Create and populate the asset
	UPackage *ClassMappingPackage = CreatePackage(*Filepath);
		
	ClassDataAsset = NewObject<UClassMappingAsset>(
		ClassMappingPackage,
		UClassMappingAsset::StaticClass(),
		*Filename,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
}

UClassMappingAsset* FSegmantizerModule::GetClassDataAsset()
{
	if (ClassDataAsset)
		return ClassDataAsset;

	const FString Filename = "SemanticClassMap";
	
	if (!LoadClassDataAsset(ModuleDirectory / Filename))
		CreateClassDataAsset(ModuleDirectory, Filename);

	return ClassDataAsset;
}

void FSegmantizerModule::ShutdownModule()
{
	if (RequestDelegateHandle.IsValid())
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(RequestDelegateHandle);
}

void FSegmantizerModule::SaveClassDataAsset() const
{
	if (ClassDataAsset)
		UEditorAssetLibrary::SaveLoadedAsset(ClassDataAsset, false);
}

void FSegmantizerModule::CaptureStart()
{
	const FDateTime Now = FDateTime::Now();

	const FCaptureRequest LitRequest{ "Lit", Now, FRequestDelegate::CreateRaw(this, &FSegmantizerModule::SetViewToLit) };
	CaptureQueue.Enqueue(LitRequest);
	
	const FCaptureRequest SemanticRequest{ "Semantic", Now, FRequestDelegate::CreateRaw(this, &FSegmantizerModule::SetViewToSemantic) };
	CaptureQueue.Enqueue(SemanticRequest);

	RequestDelegateHandle = FScreenshotRequest::OnScreenshotRequestProcessed().Add(RequestDelegate);

	CaptureLoop();
}

void FSegmantizerModule::ShotCapture(const FString& Filename, const FDateTime& DateTime) const
{
	const FString NowStr = FString::Printf(TEXT("%d-%02d-%02d-%02d-%02d-%02d-%03d"), DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay(), DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond(), DateTime.GetMillisecond());

	const FString CompleteName = Filename + NowStr;
	
	const FString Filepath = "";
	const FString CompletePath = Filepath + CompleteName;

	FHighResScreenshotConfig& HighResConfig = GetHighResScreenshotConfig();
	HighResConfig.SetResolution(0, 0);
	HighResConfig.SetFilename(CompletePath);
	HighResConfig.SetMaskEnabled(false);
	HighResConfig.bDateTimeBasedNaming = false;
	
	FScreenshotRequest::RequestScreenshot(CompletePath, false, false);
}

bool FSegmantizerModule::UnrollQueue()
{
	FCaptureRequest CurrentCapture;
	if (!CaptureQueue.Dequeue(CurrentCapture))
	{
		CaptureEnd();

		// Return false to stop the loop
		return false;
	}

	if (CurrentCapture.CaptureDelegate.IsBound())
		CurrentCapture.CaptureDelegate.Execute();

	ShotCapture(CurrentCapture.Filename, CurrentCapture.DateTime);

	// Return true to continue the loop
	return true;
}

void FSegmantizerModule::CaptureLoop()
{
	// Called each time a Screenshot is processed 
	if (!UnrollQueue())
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(RequestDelegateHandle);
}

void FSegmantizerModule::CaptureEnd()
{
	ClassManager.RestoreAll();
}

void FSegmantizerModule::SetViewToSemantic()
{
	UClassMappingAsset* CurrentClassData = GetClassDataAsset();
	
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	
	TArray<AActor*> LevelActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContext->World(), AActor::StaticClass(), LevelActors);
	
	for (AActor* Actor : LevelActors)
	{
		// Get class name from actor instance
		const FString* ClassNamePtr = CurrentClassData->ActorInstanceToClassName.Find(Actor->GetActorGuid());

		// Else get class name from actor class
		if (!ClassNamePtr)
			ClassNamePtr = CurrentClassData->ActorClassToClassName.Find(Actor->GetClass());

		if (!ClassNamePtr)
			continue;

		// Register the actor instance with the ClassManager
		// TODO: Split the registering between actor instances and actor classes to speed up the restoration
		ClassManager.AddUniqueActorInstance(Actor);
		
		FSemanticClass& SemanticClass = CurrentClassData->SemanticClasses[*ClassNamePtr];

		UMaterialInstanceConstant* Material = ClassManager.GetSemanticClassMaterial(SemanticClass);
		ClassManager.PaintActor(Actor, Material);
	}
}

void FSegmantizerModule::SetViewToLit()
{
	ClassManager.RestoreAll();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSegmantizerModule, Segmantizer)