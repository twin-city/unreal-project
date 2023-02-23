// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassNavigationTypes.h"
#include "MassCharacterMovementInfo_AnimInput.generated.h"

USTRUCT(BlueprintType)
struct FMassCharacterMovementInfo_AnimInput
{
	GENERATED_BODY()

	// Current raw desired velocity from steering.
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FVector DesiredVelocity = FVector::ZeroVector;

	// Global time in seconds when the current action started.
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	float CurrentActionStartTime = 0.0f;

	// Sequential action ID, updated as AI changes it's mind.
	uint16 CurrentActionID = 0;

	// Previous movement action.
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	EMassMovementAction PreviousMovementAction = EMassMovementAction::Move;

	// Current movement action.
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	EMassMovementAction CurrentMovementAction = EMassMovementAction::Move;
};