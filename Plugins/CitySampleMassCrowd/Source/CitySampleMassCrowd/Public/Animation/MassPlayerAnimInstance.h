// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "MassCharacterProximity_AnimInput.h"
#include "MassPlayerAnimInstance.generated.h"

UCLASS(config=Game)
class UMassPlayerAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UMassPlayerAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeInitializeAnimation();
	virtual void NativeUpdateAnimation(float DeltaSeconds);

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FMassCharacterProximity_AnimInput CrowdProximity;
};

