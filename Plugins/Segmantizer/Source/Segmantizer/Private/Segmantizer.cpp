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
	ClassManager = NewObject<UClassManager>();
	
	const FString Prefix = FPaths::ProjectPluginsDir();
	const FString Directory = Prefix / "Segmantizer" / "Content";

	const FString FileName = "SemanticClassMap";
	
	// Register the plugin directory with the editor
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().AddPath(*Directory);

	const FString Filepath = Directory / FileName;
	
	ClassDataAsset = LoadObject<UClassMappingAsset>(nullptr, *Filepath);

	if (!ClassDataAsset)
	{
		// Create and populate the asset
		UPackage *ClassMappingPackage = CreatePackage(*Filepath);
		
		ClassDataAsset = NewObject<UClassMappingAsset>(
			ClassMappingPackage,
			UClassMappingAsset::StaticClass(),
			*FileName,
			EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);

	}
}

void FSegmantizerModule::Save()
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
		FString* ClassNamePtr = ClassDataAsset->ActorInstanceToClassName.Find(Actor->GetActorGuid());
		
		if (!ClassNamePtr)
			ClassNamePtr = ClassDataAsset->ActorClassToClassName.Find(Actor->GetClass());

		if (ClassNamePtr)
		{
			FSemanticClass& SemanticClass = ClassDataAsset->SemanticClasses[*ClassNamePtr];
			GEngine->AddOnScreenDebugMessage(-1,200,SemanticClass.Color,FString::Printf(TEXT("Hello %s"), *SemanticClass.Name));
		}
	}
}

void FSegmantizerModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FSegmantizerModule, Segmantizer)