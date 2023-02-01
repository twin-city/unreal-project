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

UCLASS()
class SEGMANTIZER_API UClassManager : public UObject
{
	GENERATED_BODY()

private:
	UPROPERTY()
	TMap<AActor*, FActorDescriptor> ActorInstanceDescriptors;

public:
	void AddActorInstance(AActor* ActorInstance);
	void RemoveActorInstance(const AActor* ActorInstance);
	void PaintActor(const AActor* ToPaint, class UMaterialInstanceConstant* Material);
	void RestoreAll();
};
