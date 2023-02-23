// Copyright Epic Games, Inc. All Rights Reserved.

#include "Animation/MassPlayerAnimInstance.h"


UMassPlayerAnimInstance::UMassPlayerAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMassPlayerAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMassPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	FMassCharacterProximity_UpdateContext UpdateContext;
	UpdateContext.DeltaSeconds = DeltaSeconds;
	UpdateContext.MeshTransform = GetOwningComponent()->GetComponentTransform();
	CrowdProximity.Update(UpdateContext);
}