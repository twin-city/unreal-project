// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassSmartObjectBehaviorDefinition.h"
#include "SmartObjectMassInteractionDefinition.generated.h"

UCLASS(EditInlineNew)
class USmartObjectMassInteractionDefinition : public USmartObjectMassBehaviorDefinition
{
	GENERATED_BODY()

protected:
	virtual void Activate(FMassCommandBuffer& CommandBuffer, const FMassBehaviorEntityContext& EntityContext) const override;

	UPROPERTY(EditAnywhere, Category = Anim)
	class UContextualAnimSceneAsset* ContextualAnimAsset;

	UPROPERTY(EditAnywhere, Category = Anim)
	FName AlignmentTrack;

	UPROPERTY(EditAnywhere, Category = Anim)
	FName InteractorRole;
};
