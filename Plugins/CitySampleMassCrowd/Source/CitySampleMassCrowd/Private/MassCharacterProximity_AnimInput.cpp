// Copyright Epic Games, Inc. All Rights Reserved.
#include "Animation/MassCharacterProximity_AnimInput.h"

void FMassCharacterProximity_AnimInput::Update(const FMassCharacterProximity_UpdateContext& Context)
{
	FQuat OwnerRotation = Context.MeshTransform.GetRotation();
	FVector OwnerToOther = OtherMeshLocation - Context.MeshTransform.GetLocation();
	OwnerToOther.Z = 0.0f;
	Distance = OwnerToOther.Size();
	if (Distance > KINDA_SMALL_NUMBER)
	{
		OwnerToOther.Normalize();
		OwnerToOther = OwnerRotation.UnrotateVector(OwnerToOther);

		RelativeDirectionToOther2D = FVector2D(OwnerToOther.X, OwnerToOther.Y);
	}
	FVector RelativeOtherVelocity = OwnerRotation.UnrotateVector(FVector(OtherVelocity2D.X, OtherVelocity2D.Y, 0.0f));
	RelativeOtherVelocity2D = FVector2D(RelativeOtherVelocity.X, RelativeOtherVelocity.Y);
}
