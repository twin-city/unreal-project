// Copyright Epic Games, Inc. All Rights Reserved.

#include "Tasks/MassContextualAnimTask.h"

#include "MassSignalSubsystem.h"
#include "MassStateTreeExecutionContext.h"
#include "MassCrowdAnimationTypes.h"
#include "MassCommandBuffer.h"
#include "Animation/AnimMontage.h"
#include "MassNavigationFragments.h"
#include "MassZoneGraphNavigationUtils.h"
#include "StateTreeLinker.h"

FMassContextualAnimTask::FMassContextualAnimTask()
{
}

bool FMassContextualAnimTask::Link(FStateTreeLinker& Linker)
{
	Linker.LinkExternalData(MassSignalSubsystemHandle);
	Linker.LinkExternalData(MontageRequestHandle);
	Linker.LinkExternalData(TransformHandle);
	Linker.LinkExternalData(MoveTargetHandle);

	return true;
}

EStateTreeRunStatus FMassContextualAnimTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FMassStateTreeExecutionContext& MassContext = static_cast<FMassStateTreeExecutionContext&>(Context);

	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	InstanceData.Time = 0.f;

	UE::CrowdInteractionAnim::FRequest AnimRequest;
	AnimRequest.ContextualAnimAsset = ContextualAnimAsset;
	AnimRequest.InteractorRole = InteractorRole;
	AnimRequest.AlignmentTrack = AlignmentTrack;

	FContextualAnimQueryResult& ContextualAnimQueryResult = AnimRequest.QueryResult;
	// If we have a target entity associated, use that to find the best contextual anim to play
	if (ContextualAnimAsset != nullptr && InstanceData.TargetEntity.IsSet())
	{
		if (const FTransformFragment* TargetTransformFragment = MassContext.GetEntityManager().GetFragmentDataPtr<FTransformFragment>(InstanceData.TargetEntity))
		{
			const FTransform& TargetTransform = TargetTransformFragment->GetTransform();
			const FTransform& EntityTransform = MassContext.GetExternalData(TransformHandle).GetTransform();

			FContextualAnimQueryParams ContextualAnimQueryParams;
			ContextualAnimQueryParams.bComplexQuery = true;
			ContextualAnimQueryParams.bFindAnimStartTime = true;
			ContextualAnimQueryParams.QueryTransform = EntityTransform;

			// If we don't find a good sync point, grab the closest one.
			if (!ContextualAnimAsset->Query(InteractorRole, ContextualAnimQueryResult, ContextualAnimQueryParams, TargetTransform))
			{
				ContextualAnimQueryParams.bComplexQuery = false;
				ContextualAnimAsset->Query(InteractorRole, ContextualAnimQueryResult, ContextualAnimQueryParams, TargetTransform);
			}
		}
	}
	
	// If we didn't find a proper contextual anim, or it was not set, use a simple montage instead
	if (!ContextualAnimQueryResult.Animation.IsValid())
	{
		ContextualAnimQueryResult.Animation = FallbackMontage;
	}

	InstanceData.ComputedDuration = InstanceData.Duration;
	
	if (const UAnimMontage* Montage = ContextualAnimQueryResult.Animation.Get())
	{
		// Only override movement mode if we have root motion
		if (Montage->HasRootMotion())
		{
			const UWorld* World = Context.GetWorld();
			checkf(World != nullptr, TEXT("A valid world is expected from the execution context"));

			FMassMoveTargetFragment& MoveTarget = Context.GetExternalData(MoveTargetHandle);
			MoveTarget.CreateNewAction(EMassMovementAction::Animate, *World);
			if (!UE::MassNavigation::ActivateActionAnimate(*World, Context.GetOwner(), MassContext.GetEntity(), MoveTarget))
			{
				return EStateTreeRunStatus::Failed;
			}
		}

		// Grab the task duration from the montage.
		InstanceData.ComputedDuration = Montage->GetPlayLength();
		// Use existing fragment or push one
		FMassMontageFragment* MontageFragment = MassContext.GetExternalDataPtr(MontageRequestHandle);
		if (MontageFragment != nullptr)
		{
			MontageFragment->Request(AnimRequest);
		}
		else
		{
			FMassMontageFragment MontageData;
			MontageData.Request(AnimRequest);
			MassContext.GetEntitySubsystemExecutionContext().Defer().PushCommand<FMassCommandAddFragmentInstances>(MassContext.GetEntity(), MontageData);
		}
	}

	// A Duration <= 0 indicates that the task runs until a transition in the state tree stops it.
	// Otherwise we schedule a signal to end the task.
	if (InstanceData.ComputedDuration > 0.0f)
	{
		UMassSignalSubsystem& MassSignalSubsystem = MassContext.GetExternalData(MassSignalSubsystemHandle);
		MassSignalSubsystem.DelaySignalEntity(UE::Mass::Signals::ContextualAnimTaskFinished, MassContext.GetEntity(), InstanceData.ComputedDuration);
	}

	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FMassContextualAnimTask::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& InstanceData = Context.GetInstanceData<FInstanceDataType>(*this);

	InstanceData.Time += DeltaTime;
	return InstanceData.ComputedDuration <= 0.0f ? EStateTreeRunStatus::Running : (InstanceData.Time < InstanceData.ComputedDuration ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Succeeded);
}
