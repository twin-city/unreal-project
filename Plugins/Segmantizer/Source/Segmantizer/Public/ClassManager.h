#pragma once

#include "CoreMinimal.h"
#include "ClassManager.generated.h"

// Structure wrapping TArray of static mesh component materials
USTRUCT()
struct FComponentDescriptor
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<UMaterialInterface*> MaterialInterfaces;
};


// Structure wrapping TMap of static mesh components to their original materials
USTRUCT()
struct FActorDescriptor
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TMap<UPrimitiveComponent*, FComponentDescriptor> CompDescriptors;
};

USTRUCT()
struct SEGMANTIZER_API FClassManager
{
	GENERATED_BODY()

private:
	UPROPERTY()
	UMaterial* SemanticMaterial = nullptr;

	UPROPERTY()
	TMap<AActor*, FActorDescriptor> ActorInstanceDescriptors;

public:
	FClassManager();

	void AddActorInstance(AActor* ActorInstance);
	void AddUniqueActorInstance(AActor* ActorInstance);
	void RemoveActorInstance(const AActor* ActorInstance);
	void PaintActor(const AActor* ToPaint, class UMaterialInstanceConstant* Material);
	void RestoreAll();
	
	UMaterialInstanceConstant* GetSemanticClassMaterial(struct FSemanticClass& SemanticClass);
};
