// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassNavigationTypes.h"
#include "MassCharacterStopPrediction_AnimInput.generated.h"

USTRUCT(BlueprintType)
struct FMassCharacterStopPrediction_AnimInput
{
	GENERATED_BODY()

	// Approximate distance to end of path. This distance is the same the AI uses to handle end of path.
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	float DistanceToEndOfPath = 0.0f;

	// What the AI expects to do at the end of the path (i.e. keep on moving, stand).
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	EMassMovementAction ActionAtEndOfPath = EMassMovementAction::Move;
};