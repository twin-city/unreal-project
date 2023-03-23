// Copyright Epic Games, Inc. All Rights Reserved.

#include "Segmantizer.h"

#include "ClassManager.h"
#include "ClassMappingAsset.h"
#include "Kismet/GameplayStatics.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"
#include "Slate/SceneViewport.h"
#include "SLevelViewport.h"
#include "HighResScreenshot.h"

#define LOCTEXT_NAMESPACE "FSegmantizerModule"

void FSegmantizerModule::StartupModule()
{
	RequestDelegate = FRequestDelegate::CreateRaw(this, &FSegmantizerModule::CaptureLoop);
	TickedDelegate = FTickerDelegate::CreateRaw(this, &FSegmantizerModule::ShotTickedCapture);

	const FString Directory = TEXT("/Segmantizer");

	const FString Filename = "SemanticClassMap";

	if (!LoadClassDataAsset(Directory / Filename))
		CreateClassDataAsset(Directory, Filename);
}

bool FSegmantizerModule::LoadClassDataAsset(const FString& Filepath)
{
	ClassDataAsset = LoadObject<UClassMappingAsset>(nullptr, *Filepath);
	return ClassDataAsset != nullptr;
}

void FSegmantizerModule::CreateClassDataAsset(const FString& Directory, const FString& Filename)
{
	// Register the plugin directory with the editor
	const FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AddPath(*Directory);

	const FString Filepath = Directory / Filename;
	
	// Create and populate the asset
	UPackage *ClassMappingPackage = CreatePackage(*Filepath);
		
	ClassDataAsset = NewObject<UClassMappingAsset>(
		ClassMappingPackage,
		UClassMappingAsset::StaticClass(),
		*Filename,
		EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
}

void FSegmantizerModule::ShutdownModule()
{
	if (RequestDelegateHandle.IsValid())
		FScreenshotRequest::OnScreenshotRequestProcessed().Remove(RequestDelegateHandle);

	if (TickerDelegateHandle.IsValid())
		FTSTicker::GetCoreTicker().RemoveTicker(TickerDelegateHandle);
}

void FSegmantizerModule::SaveClassDataAsset() const
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

	//if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
	//{
	//	FLevelEditorModule& LevelEditor = FModuleManager::GetModuleChecked<FLevelEditorModule>("LevelEditor");
	//	SLevelViewport* LevelViewport = LevelEditor.GetFirstActiveLevelViewport().Get();
	//	if (!LevelViewport->IsInGameView() && LevelViewport->CanToggleGameView())
	//		LevelViewport->ToggleGameView();
	//}
	
	CaptureLoop();
}

bool FSegmantizerModule::ShotTickedCapture(float DeltaTime)
{
	ShotCapture(CurrentCapture.Filename, CurrentCapture.DateTime);

	return false;
}

void FSegmantizerModule::ShotCapture(const FString& Filename, const FDateTime& DateTime) const
{
	const FString Filepath = "C:/Users/remig/Downloads/Screenshots/";
	const FString NowStr = FString::Printf(TEXT("%d.%02d.%02d-%02d.%02d.%02d.%03d"), DateTime.GetYear(), DateTime.GetMonth(), DateTime.GetDay(), DateTime.GetHour(), DateTime.GetMinute(), DateTime.GetSecond(), DateTime.GetMillisecond());

	const FString CompletePath = Filepath + Filename + NowStr;
	
	FScreenshotRequest::RequestScreenshot(CompletePath, false, true);
}

bool FSegmantizerModule::UnrollQueue()
{
	if (!CaptureQueue.Dequeue(CurrentCapture))
	{
		CaptureEnd();
		return false;
	}

	if (CurrentCapture.CaptureDelegate.IsBound())
		CurrentCapture.CaptureDelegate.Execute();

	TickerDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickedDelegate, 0);

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

		if (!ClassNamePtr)
			continue;
		
		ClassManager.AddUniqueActorInstance(Actor);
		
		FSemanticClass& SemanticClass = ClassDataAsset->SemanticClasses[*ClassNamePtr];

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