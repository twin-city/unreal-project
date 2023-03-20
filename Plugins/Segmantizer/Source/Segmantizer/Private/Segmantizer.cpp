// Copyright Epic Games, Inc. All Rights Reserved.

#include "Segmantizer.h"

#include "ClassManager.h"
#include "ClassMappingAsset.h"
#include "Kismet/GameplayStatics.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FSegmantizerModule"

void FSegmantizerModule::StartupModule()
{
	RequestDelegate = FRequestDelegate::CreateRaw(this, &FSegmantizerModule::CaptureLoop);
	
	const FString Directory = TEXT("/Segmantizer");

	const FString FileName = "SemanticClassMap";

	const FString Filepath = Directory / FileName;
	
	ClassDataAsset = LoadObject<UClassMappingAsset>(nullptr, *Filepath);

	if (!ClassDataAsset)
	{
		// Register the plugin directory with the editor
		const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		AssetRegistryModule.Get().AddPath(*Directory);
		
		// Create and populate the asset
		UPackage *ClassMappingPackage = CreatePackage(*Filepath);
		
		ClassDataAsset = NewObject<UClassMappingAsset>(
			ClassMappingPackage,
			UClassMappingAsset::StaticClass(),
			*FileName,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	}
}

void FSegmantizerModule::ShutdownModule()
{
	if (RequestDelegateHandle.IsValid())
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(RequestDelegateHandle);
}

void FSegmantizerModule::Save()
{
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

void FSegmantizerModule::ShotCapture(const FString& Filename, const FDateTime& DateTime)
{
	const FString NowStr = FString::Printf(TEXT("%d.%02d.%02d-%02d.%02d.%02d.%03d"), DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay(), DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond(), DateTime.GetMillisecond());

	FScreenshotRequest::RequestScreenshot(Filename + NowStr, false, false);
}

bool FSegmantizerModule::UnrollQueue()
{
	FCaptureRequest CaptureRequest;
	if (!CaptureQueue.Dequeue(CaptureRequest))
	{
		CaptureEnd();
		return false;
	}

	if (CaptureRequest.CaptureDelegate.IsBound())
		CaptureRequest.CaptureDelegate.Execute();

	ShotCapture(CaptureRequest.Filename, CaptureRequest.DateTime);

	return true;
}

void FSegmantizerModule::CaptureLoop()
{
	if (!UnrollQueue())
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(RequestDelegateHandle);
}

void FSegmantizerModule::CaptureEnd()
{
	ClassManager.RestoreAll();
}

void FSegmantizerModule::SetViewToSemantic()
{
	TArray<AActor*> LevelActors;

	const FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);
	
	UGameplayStatics::GetAllActorsOfClass(WorldContext->World(), AActor::StaticClass(), LevelActors);

	for (AActor* Actor : LevelActors)
	{
		const FString* ClassNamePtr = ClassDataAsset->ActorInstanceToClassName.Find(Actor->GetActorGuid());
		
		if (!ClassNamePtr)
			ClassNamePtr = ClassDataAsset->ActorClassToClassName.Find(Actor->GetClass());

		if (ClassNamePtr)
		{
			ClassManager.AddUniqueActorInstance(Actor);
			
			FSemanticClass& SemanticClass = ClassDataAsset->SemanticClasses[*ClassNamePtr];

			UMaterialInstanceConstant* Material = ClassManager.GetSemanticClassMaterial(SemanticClass);
			ClassManager.PaintActor(Actor, Material);
		}
	}
}

void FSegmantizerModule::SetViewToLit()
{
	ClassManager.RestoreAll();
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSegmantizerModule, Segmantizer)