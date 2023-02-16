// Fill out your copyright notice in the Description page of Project Settings.


#include "ClassManager.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "ClassMappingAsset.h"

FClassManager::FClassManager()
{
	UMaterial* AssetMaterial = LoadObject<UMaterial>(nullptr, *FString("/Segmantizer/SemanticMaterial"));
	SemanticMaterial = DuplicateObject<UMaterial>(AssetMaterial, nullptr);
}

void FClassManager::AddActorInstance(AActor* ActorInstance)
{
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

void FClassManager::RemoveActorInstance(const AActor* ActorInstance)
{
	ActorInstanceDescriptors.Remove(ActorInstance);
}

void FClassManager::PaintActor(const AActor* ToPaint, UMaterialInstanceConstant* Material)
{
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

UMaterialInstanceConstant* FClassManager::GetSemanticClassMaterial(FSemanticClass& SemanticClass)
{
	if (SemanticClass.PlainColorMaterialInstance)
		return SemanticClass.PlainColorMaterialInstance;
		
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = SemanticMaterial;

	const FString PackageFileName = FString::Printf(TEXT("MI_%s"), *(SemanticClass.Name));
	const FString PackagePath = TEXT("/Segmantizer/ConstMaterials") / PackageFileName;
	
	UPackage* Package = CreatePackage(*PackagePath);
	
	SemanticClass.PlainColorMaterialInstance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(),
		Package,
		*PackageFileName,
		RF_Public | RF_Transient,
		nullptr,
		GWarn));

	if (!SemanticClass.PlainColorMaterialInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("%s: Could not create the plain color material instance"), *FString(__FUNCTION__))
		check(SemanticClass.PlainColorMaterialInstance)
	}
	
	SemanticClass.PlainColorMaterialInstance->SetVectorParameterValueEditorOnly(TEXT("SemanticColor"), SemanticClass.Color);

	return SemanticClass.PlainColorMaterialInstance;
}