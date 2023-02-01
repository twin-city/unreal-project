// Fill out your copyright notice in the Description page of Project Settings.


#include "ClassManager.h"
#include "Materials/MaterialInstanceConstant.h"

void UClassManager::AddActorInstance(AActor* ActorInstance)
{
	TArray<UActorComponent*> ActorComponents;
	constexpr bool bIncludeFromChildActors = true;
	ActorInstance->GetComponents(UPrimitiveComponent::StaticClass(), ActorComponents, bIncludeFromChildActors);

	FActorDescriptor& ActorDescriptor = ActorInstanceDescriptors.Emplace(ActorInstance);
	
	for (UActorComponent* ActorComponent : ActorComponents)
	{
		UPrimitiveComponent* PrimitiveComponent = Cast<UPrimitiveComponent>(ActorComponent);
		FComponentDescriptor& CompDescriptor = ActorDescriptor.CompDescriptors.Emplace(PrimitiveComponent);

		// Save original material
		for (int i = 0; i < PrimitiveComponent->GetNumMaterials(); i++)
			CompDescriptor.MaterialInterfaces.Add(PrimitiveComponent->GetMaterial(i));
	}
}

void UClassManager::RemoveActorInstance(const AActor* ActorInstance)
{
	ActorInstanceDescriptors.Remove(ActorInstance);
}

void UClassManager::PaintActor(const AActor* ToPaint, UMaterialInstanceConstant* Material)
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

void UClassManager::RestoreAll()
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