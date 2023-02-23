// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassCharacterProximity_AnimInput.generated.h"

struct FMassCharacterProximity_UpdateContext
{
	FTransform MeshTransform;
	float DeltaSeconds;
};

USTRUCT(BlueprintType)
struct FMassCharacterProximity_AnimInput
{
	GENERATED_BODY()

	UPROPERTY(transient, BlueprintReadOnly, Category = CharacterProximity)
	FVector OtherMeshLocation = FVector::ZeroVector;

	UPROPERTY(transient, BlueprintReadOnly, Category = CharacterProximity)
	FRotator OtherMeshRotation = FRotator::ZeroRotator;

	UPROPERTY(transient, BlueprintReadOnly, Category = CharacterProximity)
	FVector2D OtherVelocity2D = FVector2D::ZeroVector;

	UPROPERTY(transient, BlueprintReadOnly, Category = CharacterProximity)
	FVector2D RelativeDirectionToOther2D = FVector2D::Unit45Deg;

	UPROPERTY(transient, BlueprintReadOnly, Category = CharacterProximity)
	FVector2D RelativeOtherVelocity2D = FVector2D::Unit45Deg;

	UPROPERTY(transient, BlueprintReadOnly, EditDefaultsOnly, Category = CharacterProximity)
	float Distance = 0.0f;

	void Update(const FMassCharacterProximity_UpdateContext& Context);
};