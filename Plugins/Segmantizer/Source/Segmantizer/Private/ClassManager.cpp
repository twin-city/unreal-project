// Fill out your copyright notice in the Description page of Project Settings.

#include "ClassManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "ClassMappingAsset.h"
#include "EditorAssetLibrary.h"
#include "Engine/AssetManager.h"

FClassManager::FClassManager()
{
	SemanticMaterial = LoadObject<UMaterial>(nullptr, *FString("/Segmantizer/SemanticMaterial"));
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
	auto& ActorInstanceDescriptors = GetCurrentWorldDescriptor().ActorInstanceDescriptors;

	if (!ActorInstanceDescriptors.Contains(ActorInstance))
		AddActorInstance(ActorInstance);
}

void FClassManager::RemoveActorInstance(AActor* ActorInstance)
{
	RemoveActorConstInstance(ActorInstance);
}

void FClassManager::RemoveActorConstInstance(const AActor* ActorInstance)
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

UMaterialInstanceConstant* FClassManager::GetSemanticClassMaterial(FSemanticClass& SemanticClass)
{
	if (SemanticClass.PlainColorMaterialInstance)
		return SemanticClass.PlainColorMaterialInstance;

	const FString PackageFileName = FString::Printf(TEXT("MI_%s"), *(SemanticClass.Name));
	const FString PackagePath = TEXT("/Segmantizer/ConstMaterials") / PackageFileName;

	UObject* LoadedAsset = UAssetManager::GetStreamableManager().LoadSynchronous(PackagePath);

	if (UMaterialInstanceConstant* InstanceConstant = Cast<UMaterialInstanceConstant>(LoadedAsset))
		return SemanticClass.PlainColorMaterialInstance = InstanceConstant;
	
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
	
	NewInstanceConstant->SetVectorParameterValueEditorOnly(TEXT("SemanticColor"), SemanticClass.Color);
	UEditorAssetLibrary::SaveLoadedAsset(NewInstanceConstant, false);

	SemanticClass.PlainColorMaterialInstance = NewInstanceConstant;
	
	if (!SemanticClass.PlainColorMaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Could not create the plain color material instance"), *FString(__FUNCTION__))
		check(SemanticClass.PlainColorMaterialInstance)
	}

	return SemanticClass.PlainColorMaterialInstance;
}