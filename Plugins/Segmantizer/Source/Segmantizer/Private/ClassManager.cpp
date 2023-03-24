// Fill out your copyright notice in the Description page of Project Settings.

#include "ClassManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "ClassMappingAsset.h"
#include "EditorAssetLibrary.h"
#include "Segmantizer.h"
#include "Engine/AssetManager.h"

const FString MaterialFormat = TEXT("MI_{0}");
const FString ColorParameterName = TEXT("SemanticColor");
const FString MaterialsPath = TEXT("/Segmantizer/ConstMaterials");
const FString ParentMaterialName = TEXT("SemanticMaterial");

FClassManager::FClassManager()
{
	
	SemanticMaterial = LoadObject<UMaterial>(nullptr, *(FSegmantizerModule::ModuleDirectory / ParentMaterialName));
}

FWorldDescriptor& FClassManager::GetCurrentWorldDescriptor()
{
	const FWorldContext* WorldContext = GEngine->GetWorldContextFromGameViewport(GEngine->GameViewport);

	// TODO: Clean up invalid worlds
	return WorldDescriptors.FindOrAdd(WorldContext->World());
}

void FClassManager::AddActorInstance(AActor* ActorInstance)
{
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;
	
	FActorDescriptor& ActorDescriptor = ActorInstanceDescriptors.Emplace(ActorInstance);
	
	TArray<UActorComponent*> ActorComponents;
	ActorInstance->GetComponents(UPrimitiveComponent::StaticClass(), ActorComponents, true);

	for (UActorComponent* ActorComponent : ActorComponents)
	{
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent);
		FComponentDescriptor& CompDescriptor = ActorDescriptor.CompDescriptors.Emplace(PrimitiveComponent);

		// Save original material
		for (int i = 0; i < PrimitiveComponent->GetNumMaterials(); i++)
			CompDescriptor.MaterialInterfaces.Add(PrimitiveComponent->GetMaterial(i));
	}
}

void FClassManager::AddUniqueActorInstance(AActor* ActorInstance)
{
	const FWorldDescriptor& WorldDescriptor = GetCurrentWorldDescriptor();

	if (!WorldDescriptor.ActorInstanceDescriptors.Contains(ActorInstance))
		AddActorInstance(ActorInstance);
}

void FClassManager::RemoveActorInstance(const AActor* ActorInstance)
{
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;

	ActorInstanceDescriptors.Remove(ActorInstance);
}

void FClassManager::PaintActor(const AActor* ToPaint, UMaterialInstanceConstant* Material)
{
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;

	UMaterialInterface* MaterialInterface = Cast<UMaterialInterface>(Material);
	
	FActorDescriptor& ActorDescriptor = ActorInstanceDescriptors[ToPaint];
	
	for (TTuple<UPrimitiveComponent*, FComponentDescriptor>& CompTuple : ActorDescriptor.CompDescriptors)
	{
		UPrimitiveComponent* Component = CompTuple.Key;
		FComponentDescriptor& ComponentDescriptor = CompTuple.Value;

		for (int mi = 0; mi < ComponentDescriptor.MaterialInterfaces.Num(); mi++)
			Component->SetMaterial(mi, MaterialInterface);
	}
}

void FClassManager::RestoreAll()
{
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;
	
	for (TTuple<AActor*, FActorDescriptor>& ActorTuple : ActorInstanceDescriptors)
	{
		FActorDescriptor& ActorDescriptor = ActorTuple.Value;
		
		for (TTuple<UPrimitiveComponent*, FComponentDescriptor>& CompTuple : ActorDescriptor.CompDescriptors)
		{
			UPrimitiveComponent* Component = CompTuple.Key;
			FComponentDescriptor& ComponentDescriptor = CompTuple.Value;

			// Set back all materials of the current PrimitiveComponent
			for (int mi = 0; mi < ComponentDescriptor.MaterialInterfaces.Num(); mi++)
				Component->SetMaterial(mi, ComponentDescriptor.MaterialInterfaces[mi]);
		}
	}
}

void FClassManager::Clear()
{
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;

	ActorInstanceDescriptors.Empty(ActorInstanceDescriptors.Num());
}

UMaterialInstanceConstant* FClassManager::LoadSemanticClassMaterial(FSemanticClass& SemanticClass) const
{
	const FString PackageName = FString::Format(*MaterialFormat, { SemanticClass.Name });
	const FString PackagePath = MaterialsPath / PackageName;
	
	UObject* LoadedAsset = UAssetManager::GetStreamableManager().LoadSynchronous(PackagePath);

	// Return the material instance or nullptr
	return SemanticClass.PlainColorMaterialInstance = Cast<UMaterialInstanceConstant>(LoadedAsset);
}

UMaterialInstanceConstant* FClassManager::CreateSemanticClassMaterial(FSemanticClass& SemanticClass) const
{
	const FString PackageFileName = FString::Format(*MaterialFormat, { SemanticClass.Name });
	const FString PackagePath = MaterialsPath / PackageFileName;
	
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = SemanticMaterial;
	
	UPackage* Package = CreatePackage(*PackagePath);

	UMaterialInstanceConstant* NewInstanceConstant = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(),
		Package,
		*PackageFileName,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn));

	// Set the material color with the semantic class color
	NewInstanceConstant->SetVectorParameterValueEditorOnly(*ColorParameterName, SemanticClass.Color);

	// Call PostEditChange to propagate changes to other materials in the chain
	NewInstanceConstant->PostEditChange();

	// Save the package
	UEditorAssetLibrary::SaveLoadedAsset(NewInstanceConstant, false);

	return SemanticClass.PlainColorMaterialInstance = NewInstanceConstant;
}

UMaterialInstanceConstant* FClassManager::GetSemanticClassMaterial(FSemanticClass& SemanticClass) const
{
	// If the material is valid, return it
	if (SemanticClass.PlainColorMaterialInstance)
		return SemanticClass.PlainColorMaterialInstance;

	// Else try to load it from the asset manager
	if (UMaterialInstanceConstant* SemanticMaterialInstance = LoadSemanticClassMaterial(SemanticClass))
		return SemanticMaterialInstance;

	// Else try to create it and save it
	if (UMaterialInstanceConstant* SemanticMaterialInstance = CreateSemanticClassMaterial(SemanticClass))
		return SemanticMaterialInstance;
	
	if (!SemanticClass.PlainColorMaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Could not create the plain color material instance"), *FString(__FUNCTION__))
		check(SemanticClass.PlainColorMaterialInstance)
	}

	return nullptr;
}
