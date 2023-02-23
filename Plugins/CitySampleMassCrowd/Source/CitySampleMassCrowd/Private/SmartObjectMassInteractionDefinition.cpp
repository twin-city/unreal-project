// Copyright Epic Games, Inc. All Rights Reserved.

#include "SmartObjectMassInteractionDefinition.h"

#include "SmartObjectSubsystem.h"
#include "Animation/AnimMontage.h"
#include "MassCrowdAnimationTypes.h"
#include "MassCommandBuffer.h"
#include "MassSmartObjectFragments.h"

void USmartObjectMassInteractionDefinition::Activate(FMassCommandBuffer& CommandBuffer, const FMassBehaviorEntityContext& EntityContext) const
{
	if (ContextualAnimAsset == nullptr)
	{
		Super::Activate(CommandBuffer, EntityContext);
		return;
	}

	const FMassEntityHandle& Entity = EntityContext.EntityView.GetEntity();
	FMassSmartObjectUserFragment& SOUser = EntityContext.EntityView.GetFragmentData<FMassSmartObjectUserFragment>();
	FTransformFragment& TransformFragment = EntityContext.EntityView.GetFragmentData<FTransformFragment>();
	USmartObjectSubsystem& Subsystem = EntityContext.SmartObjectSubsystem;

	const FTransform SlotTransform = Subsystem.GetSlotTransform(SOUser.InteractionHandle).Get(FTransform::Identity);

	FRotator R = SlotTransform.Rotator();
	R -= FRotator::MakeFromEuler(FVector(0.f, 0.f, 90.f));
	FTransform SOTransform = FTransform(R, SlotTransform.GetTranslation(), FVector::One());

	UE::CrowdInteractionAnim::FRequest AnimRequest;
	AnimRequest.ContextualAnimAsset = ContextualAnimAsset;
	AnimRequest.InteractorRole = InteractorRole;
	AnimRequest.AlignmentTrack = AlignmentTrack;

	FContextualAnimQueryResult& ContextualAnimQueryResult = AnimRequest.QueryResult;
	const FTransform& EntityTransform = TransformFragment.GetTransform();
	
	FContextualAnimQueryParams ContextualAnimQueryParams;
	ContextualAnimQueryParams.bComplexQuery = true;
	ContextualAnimQueryParams.bFindAnimStartTime = true;
	ContextualAnimQueryParams.QueryTransform = EntityTransform;
	
	if (!ContextualAnimAsset->Query(InteractorRole, ContextualAnimQueryResult, ContextualAnimQueryParams, SOTransform))
	{
		ContextualAnimQueryParams.bComplexQuery = false;
		ContextualAnimAsset->Query(InteractorRole, ContextualAnimQueryResult, ContextualAnimQueryParams, SOTransform);
	}

	FMassSmartObjectTimedBehaviorFragment TimedBehaviorFragment;
	UAnimMontage* EntryMontage = ContextualAnimQueryResult.Animation.Get();
	TimedBehaviorFragment.UseTime = (EntryMontage ? EntryMontage->GetPlayLength() : UseTime);

	// Use existing fragment or push one
	FMassMontageFragment* MontageFragment = EntityContext.EntityView.GetFragmentDataPtr<FMassMontageFragment>();
	if (MontageFragment != nullptr)
	{
		MontageFragment->Request(AnimRequest);
		CommandBuffer.PushCommand<FMassCommandAddFragmentInstances>(Entity, TimedBehaviorFragment);
	}
	else
	{
		FMassMontageFragment MontageData;
		MontageData.Request(AnimRequest);
		CommandBuffer.PushCommand<FMassCommandAddFragmentInstances>(Entity, MontageData, TimedBehaviorFragment);
	}
}
