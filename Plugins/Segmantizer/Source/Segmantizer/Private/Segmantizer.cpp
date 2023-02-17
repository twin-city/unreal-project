// Copyright Epic Games, Inc. All Rights Reserved.

#include "Segmantizer.h"

#include "ClassManager.h"
#include "ClassMappingAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Editor/EditorEngine.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FSegmantizerModule"

void FSegmantizerModule::StartupModule()
{
	TickDelegate = FTickerDelegate::CreateRaw(this, &FSegmantizerModule::CaptureLoop);
	
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
	if (TickDelegateHandle.IsValid())
		FTSTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
}

void FSegmantizerModule::Save()
{
	UEditorAssetLibrary::SaveLoadedAsset(ClassDataAsset, false);
}

void FSegmantizerModule::CaptureStart()
{
	const FCaptureRequest SemanticRequest{ "Semantic", FRequestDelegate::CreateRaw(this, &FSegmantizerModule::SetViewToSemantic) };
	CaptureQueue.Enqueue(SemanticRequest);
	
	const FCaptureRequest LitRequest{ "Lit", FRequestDelegate::CreateRaw(this, &FSegmantizerModule::SetViewToLit) };
	CaptureQueue.Enqueue(LitRequest);

	TickDelegateHandle = FTSTicker::GetCoreTicker().AddTicker(TickDelegate);
}

void FSegmantizerModule::ShotCapture(const FString& Filename)
{
	const FDateTime Now = FDateTime::Now();
	const FString NowStr = FString::Printf(TEXT("%d.%02d.%02d-%02d.%02d.%02d.%03d"), Now.GetYear(), Now.GetMonth(), Now.GetDay(), Now.GetHour(), Now.GetMinute(), Now.GetSecond(), Now.GetMillisecond());

	FScreenshotRequest::RequestScreenshot(Filename + NowStr, false, false);
}

bool FSegmantizerModule::CaptureLoop(float DeltaTime)
{
	FCaptureRequest CaptureRequest;
	if (!CaptureQueue.Dequeue(CaptureRequest))
	{
		CaptureEnd();
		return false;
	}

	if (CaptureRequest.CaptureDelegate.IsBound())
		CaptureRequest.CaptureDelegate.Execute();

	ShotCapture(CaptureRequest.Filename);

	return true;
}

void FSegmantizerModule::CaptureEnd()
{
	
}

void FSegmantizerModule::SetViewToSemantic()
{
	TArray<AActor*> LevelActors;

	if (UEditorEngine *EEngine = Cast<UEditorEngine>(GEngine))
		UGameplayStatics::GetAllActorsOfClass(EEngine->GetEditorWorldContext().World(), AActor::StaticClass(), LevelActors);
	else 
		UGameplayStatics::GetAllActorsOfClass(GEngine->GetWorld(), AActor::StaticClass(), LevelActors);

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