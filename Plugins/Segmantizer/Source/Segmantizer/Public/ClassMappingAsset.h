// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ClassMappingAsset.generated.h"

USTRUCT(BlueprintType)
struct FSemanticClass
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere)
	FString Name;

	UPROPERTY(EditAnywhere)
	FColor Color;
};

UCLASS(BlueprintType)
class SEGMANTIZER_API UClassMappingAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Created semantic classes */
	UPROPERTY(EditAnywhere)
	TMap<FString, FSemanticClass> SemanticClasses;

	/** Actor to semantic class name bindings */
	UPROPERTY(EditAnywhere)
	TMap<FGuid, FString> ActorInstanceToClassName;

	UPROPERTY(EditAnywhere)
	TMap<UClass*, FString> ActorClassToClassName;

	void AddActorInstance(AActor* AInstance);
	void AddActorClass(UClass* AClass);
};
