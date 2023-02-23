// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassCrowdAnimationProcessor.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "AnimToTextureInstancePlaybackHelpers.h"
#include "MassCrowdRepresentationSubsystem.h"
#include "MassVisualizationComponent.h"
#include "MassRepresentationFragments.h"
#include "ContextualAnimSceneAsset.h"
#include "AnimToTextureDataAsset.h"
#include "MassActorSubsystem.h"
#include "MassCommonFragments.h"
#include "MassCrowdAnimationTypes.h"
#include "MassLookAtFragments.h"
#include "MassLODFragments.h"
#include "MassRepresentationTypes.h"
#include "MotionWarpingComponent.h"
#include "Animation/MassCrowdAnimInstance.h"
#include "Animation/MassPlayerAnimInstance.h"
#include "MassEntityView.h"
#include "MassAIBehaviorTypes.h"
#include "MassNavigationFragments.h"
#include "Steering/MassSteeringFragments.h"
#include "MassMovementFragments.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UMassFragmentInitializer_Animation::ConfigureQueries()
{
	EntityQuery.AddRequirement<FCrowdAnimationFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassFragmentInitializer_Animation::Initialize(UObject& Owner)
{
	World = Owner.GetWorld();
}

void UMassFragmentInitializer_Animation::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	check(World);
	const float GlobalTime = World->GetTimeSeconds();
	EntityQuery.ForEachEntityChunk(EntityManager, Context, [this, GlobalTime](FMassExecutionContext& Context)
		{
			TArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetMutableFragmentView<FCrowdAnimationFragment>();
			const int32 NumEntities = Context.GetNumEntities();
			for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
			{
				FCrowdAnimationFragment& AnimationData = AnimationDataList[EntityIdx];
				// @todo: Replace this range w/ the length of the starting animation states.
				const float StartTimeOffset = FMath::FRandRange(0.0f, 10.0f);
				AnimationData.GlobalStartTime = GlobalTime - StartTimeOffset;
			}
		});
}

UMassFragmentInitializer_Animation::UMassFragmentInitializer_Animation()
	: EntityQuery(*this)
{
	ObservedType = FCrowdAnimationFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;

	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
}

UMassProcessor_Animation::UMassProcessor_Animation()
	: AnimationEntityQuery_Conditional(*this)
	, MontageEntityQuery(*this)
	, MontageEntityQuery_Conditional(*this)
{
	ExecutionFlags = (int32)(EProcessorExecutionFlags::Client | EProcessorExecutionFlags::Standalone);
	ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::Tasks;
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::SyncWorldToMass);
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::Representation);

	bRequiresGameThreadExecution = true;
}

void UMassProcessor_Animation::UpdateAnimationFragmentData(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float GlobalTime, TArray<FMassEntityHandle, TInlineAllocator<32>>& ActorEntities)
{
	TArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetMutableFragmentView<FCrowdAnimationFragment>();
	TConstArrayView<FMassMontageFragment> MontageDataList = Context.GetFragmentView<FMassMontageFragment>();
	TConstArrayView<FMassRepresentationFragment> VisualizationList = Context.GetFragmentView<FMassRepresentationFragment>();
	TConstArrayView<FMassActorFragment> ActorInfoList = Context.GetFragmentView<FMassActorFragment>();

	const int32 NumEntities = Context.GetNumEntities();
	for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
	{
		FCrowdAnimationFragment& AnimationData = AnimationDataList[EntityIdx];
		const FMassRepresentationFragment& Visualization = VisualizationList[EntityIdx];
		const FMassActorFragment& ActorFragment = ActorInfoList[EntityIdx];

		if (!ActorFragment.IsOwnedByMass())
		{
			continue;
		}

		const bool bWasActor = (Visualization.PrevRepresentation == EMassRepresentationType::HighResSpawnedActor) || (Visualization.PrevRepresentation == EMassRepresentationType::LowResSpawnedActor);
		const bool bIsActor = (Visualization.CurrentRepresentation == EMassRepresentationType::HighResSpawnedActor) || (Visualization.CurrentRepresentation == EMassRepresentationType::LowResSpawnedActor);
		AnimationData.bSwappedThisFrame = (bWasActor != bIsActor);

		if (!MontageDataList.IsEmpty() && MontageDataList[EntityIdx].MontageInstance.SequenceChangedThisFrame())
		{
			AnimationData.GlobalStartTime = GlobalTime - MontageDataList[EntityIdx].MontageInstance.GetPositionInSection();
		}

		switch (Visualization.CurrentRepresentation)
		{
		case EMassRepresentationType::LowResSpawnedActor:
		case EMassRepresentationType::HighResSpawnedActor:
		{
			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			ActorEntities.Add(Entity);
			break;
		}
		default:
			break;
		}
	}
}

void UMassProcessor_Animation::UpdateVertexAnimationState(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float GlobalTime)
{
	const int32 NumEntities = Context.GetNumEntities();
	TArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetMutableFragmentView<FCrowdAnimationFragment>();
	TConstArrayView<FMassMontageFragment> MontageDataList = Context.GetFragmentView<FMassMontageFragment>();
	TConstArrayView<FMassRepresentationFragment> VisualizationList = Context.GetFragmentView<FMassRepresentationFragment>();
	TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();
	TConstArrayView<FMassVelocityFragment> VelocityList = Context.GetFragmentView<FMassVelocityFragment>();

	for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
	{
		FCrowdAnimationFragment& AnimationData = AnimationDataList[EntityIdx];

		const FMassRepresentationFragment& Visualization = VisualizationList[EntityIdx];
		const FMassVelocityFragment& Velocity = VelocityList[EntityIdx];

		// Need current anim state to update for skeletal meshes to do a smooth blend between poses
		if (Visualization.CurrentRepresentation != EMassRepresentationType::None)
		{
			int32 StateIndex = 0;

			FMassEntityHandle Entity = Context.GetEntity(EntityIdx);
			const UAnimSequence* Sequence = MontageDataList.IsEmpty() ? nullptr : MontageDataList[EntityIdx].MontageInstance.GetSequence();
			if (Sequence)
			{
				StateIndex = AnimationData.AnimToTextureData.IsValid() ? AnimationData.AnimToTextureData->GetIndexFromAnimSequence(Sequence) : 0;
			}
			else
			{
				// @todo: Make a better way to map desired anim states here. Currently the anim texture index to access is hard-coded.
				const float VelocitySizeSq = Velocity.Value.SizeSquared();
				const bool bIsWalking = Velocity.Value.SizeSquared() > MoveThresholdSq;
				if(bIsWalking)
				{
					StateIndex = 1;
					const float AuthoredAnimSpeed = 140.0f;
					const float PrevPlayRate = AnimationData.PlayRate;
					AnimationData.PlayRate = FMath::Clamp(FMath::Sqrt(VelocitySizeSq / (AuthoredAnimSpeed * AuthoredAnimSpeed)), 0.8f, 2.0f);

					// Need to conserve current frame on a playrate switch so (GlobalTime - Offset1) * Playrate1 == (GlobalTime - Offset2) * Playrate2
					AnimationData.GlobalStartTime = GlobalTime - PrevPlayRate * (GlobalTime - AnimationData.GlobalStartTime) / AnimationData.PlayRate;
				}
				else
				{
					AnimationData.PlayRate = 1.0f;
					StateIndex = 0;
				}
			}
			AnimationData.AnimationStateIndex = StateIndex;
		}
	}
}

void UMassProcessor_Animation::UpdateSkeletalAnimation(FMassEntityManager& EntityManager, float GlobalTime, TArrayView<FMassEntityHandle> ActorEntities)
{
	if (ActorEntities.Num() <= 0)
	{
		return;
	}

	// Grab player's spatial data
	FVector PlayerMeshLocation = FVector::ZeroVector;
	FRotator PlayerMeshRotation = FRotator::ZeroRotator;
	FVector2D PlayerVelocity2D = FVector2D::ZeroVector;

	// Assume single player
	if (const ACharacter* PlayerChar = UGameplayStatics::GetPlayerCharacter(this, 0))
	{
		UMassPlayerAnimInstance* PlayerAnimInstance = nullptr;
		if (const USkeletalMeshComponent* PlayerMesh = PlayerChar->GetMesh())
		{
			PlayerMeshLocation = PlayerMesh->GetComponentLocation();
			PlayerMeshRotation = PlayerMesh->GetComponentRotation();
			PlayerAnimInstance = Cast<UMassPlayerAnimInstance>(PlayerMesh->GetAnimInstance());
		}
		FVector PlayerVelocity = PlayerChar->GetVelocity();
		PlayerVelocity2D = FVector2D(PlayerVelocity.X, PlayerVelocity.Y);

		if (PlayerAnimInstance)
		{
			const ACharacter* ClosestCharacter = nullptr;
			float MinDistSq = INFINITY;
			for (FMassEntityHandle& Entity : ActorEntities)
			{
				FMassEntityView EntityView(EntityManager, Entity);

				const FMassActorFragment& ActorFragment = EntityView.GetFragmentData<FMassActorFragment>();

				if (const ACharacter* OtherCharacter = Cast<ACharacter>(ActorFragment.Get()))
				{
					float NewDistSq = FVector::DistSquared(OtherCharacter->GetActorLocation(), PlayerMeshLocation);
					if (NewDistSq < MinDistSq)
					{
						MinDistSq = NewDistSq;
						ClosestCharacter = OtherCharacter;
					}
				}
			}

			if (ClosestCharacter)
			{
				if (const USkeletalMeshComponent* OtherMesh = ClosestCharacter->GetMesh())
				{
					PlayerAnimInstance->CrowdProximity.OtherMeshLocation = OtherMesh->GetComponentLocation();
					PlayerAnimInstance->CrowdProximity.OtherMeshRotation = OtherMesh->GetComponentRotation();
				}

				FVector OtherVelocity = ClosestCharacter->GetVelocity();
				PlayerAnimInstance->CrowdProximity.OtherVelocity2D = FVector2D(OtherVelocity.X, OtherVelocity.Y);
			}
		}
	}

	for (FMassEntityHandle& Entity : ActorEntities)
	{
		FMassEntityView EntityView(EntityManager, Entity);

		FCrowdAnimationFragment& AnimationData = EntityView.GetFragmentData<FCrowdAnimationFragment>();
		FTransformFragment& TransformFragment = EntityView.GetFragmentData<FTransformFragment>();
		FMassRepresentationFragment& Visualization = EntityView.GetFragmentData<FMassRepresentationFragment>();

		const FMassActorFragment& ActorFragment = EntityView.GetFragmentData<FMassActorFragment>();
		const FMassLookAtFragment* LookAtFragment = EntityView.GetFragmentDataPtr<FMassLookAtFragment>();
		const FMassMoveTargetFragment* MovementTargetFragment = EntityView.GetFragmentDataPtr<FMassMoveTargetFragment>();
		const FMassSteeringFragment* SteeringFragment = EntityView.GetFragmentDataPtr<FMassSteeringFragment>();

		const AActor* Actor = ActorFragment.Get();
		UAnimInstance* AnimInstance = GetAnimInstanceFromActor(Actor);

		// If we're using a mass anim instance, pass the data we need.
		// @todo: This could potentially cause problems if it happens during an animation update
		if (UMassCrowdAnimInstance* MassAnimInstance = Cast<UMassCrowdAnimInstance>(AnimInstance))
		{
			FMassCrowdAnimInstanceData AnimInstanceData;
			AnimInstanceData.MassEntityTransform = TransformFragment.GetTransform();
			AnimInstanceData.LookAtDirection = LookAtFragment ? LookAtFragment->Direction : FVector::ForwardVector;
			
			AnimInstanceData.FarLODAnimSequence = nullptr;
			AnimInstanceData.FarLODPlaybackStartTime = 0.0f;
			if (AnimationData.AnimToTextureData.IsValid() && AnimationData.AnimToTextureData->Animations.IsValidIndex(AnimationData.AnimationStateIndex))
			{
				AnimInstanceData.FarLODAnimSequence = AnimationData.AnimToTextureData->AnimSequences[AnimationData.AnimationStateIndex].AnimSequence;
				if (AnimInstanceData.FarLODAnimSequence)
				{
					AnimInstanceData.FarLODAnimSequence = AnimationData.AnimToTextureData->AnimSequences[AnimationData.AnimationStateIndex].AnimSequence;

					const float SequenceLength = AnimInstanceData.FarLODAnimSequence->GetPlayLength();
					AnimInstanceData.FarLODPlaybackStartTime = FMath::Fmod(AnimationData.GlobalStartTime - GlobalTime,SequenceLength);

					if (AnimInstanceData.FarLODPlaybackStartTime < 0.0f)
					{
						AnimInstanceData.FarLODPlaybackStartTime += SequenceLength;
					}
				}
			}

			AnimInstanceData.Significance = EntityView.GetFragmentData<FMassRepresentationLODFragment>().LODSignificance;
			AnimInstanceData.bSwappedThisFrame = AnimationData.bSwappedThisFrame;

			MassAnimInstance->SetMassAnimInstanceData(AnimInstanceData);

			MassAnimInstance->PlayerProximityData.OtherMeshLocation = PlayerMeshLocation;
			MassAnimInstance->PlayerProximityData.OtherMeshRotation = PlayerMeshRotation;
			MassAnimInstance->PlayerProximityData.OtherVelocity2D = PlayerVelocity2D;

			if (SteeringFragment)
			{
				MassAnimInstance->MassMovementInfo.DesiredVelocity = SteeringFragment->DesiredVelocity;
			}

			if (MovementTargetFragment)
			{
				MassAnimInstance->MassMovementInfo.CurrentActionStartTime = MovementTargetFragment->GetCurrentActionStartTime();
				MassAnimInstance->MassMovementInfo.CurrentActionID = MovementTargetFragment->GetCurrentActionID();
				MassAnimInstance->MassMovementInfo.PreviousMovementAction = MovementTargetFragment->GetPreviousAction();
				MassAnimInstance->MassMovementInfo.CurrentMovementAction = MovementTargetFragment->GetCurrentAction();

				MassAnimInstance->StopPredictionData.DistanceToEndOfPath = MovementTargetFragment->DistanceToGoal;
				MassAnimInstance->StopPredictionData.ActionAtEndOfPath = MovementTargetFragment->IntentAtGoal;
			}
		}
		
		const FMassMontageFragment* MontageFragment = EntityManager.GetFragmentDataPtr<FMassMontageFragment>(Entity);
		UAnimMontage* Montage = MontageFragment ? MontageFragment->MontageInstance.GetMontage() : nullptr;

		if (Montage == nullptr)
		{
			continue;
		}

		if (AnimInstance && Actor)
		{
			// Don't play the montage again, even if it's blending out. UAnimInstance::GetCurrentActiveMontage and AnimInstance::Montage_IsPlaying return false if the montage is blending out.
			bool bMontageAlreadyPlaying = false;
			for (int32 InstanceIndex = 0; InstanceIndex < AnimInstance->MontageInstances.Num(); InstanceIndex++)
			{
				FAnimMontageInstance* MontageInstance = AnimInstance->MontageInstances[InstanceIndex];
				if (MontageInstance && MontageInstance->Montage == Montage && MontageInstance->IsPlaying())
				{
					bMontageAlreadyPlaying = true;
				}
			}

			if (!bMontageAlreadyPlaying)
			{
				UMotionWarpingComponent* MotionWarpingComponent = Actor->FindComponentByClass<UMotionWarpingComponent>();
				if (MotionWarpingComponent && MontageFragment->InteractionRequest.AlignmentTrack != NAME_None)
				{
					const FName SyncPointName = MontageFragment->InteractionRequest.AlignmentTrack;
					const FTransform& SyncTransform = MontageFragment->InteractionRequest.QueryResult.SyncTransform;
					MotionWarpingComponent->AddOrUpdateWarpTargetFromTransform(SyncPointName, SyncTransform);
				}

				FAlphaBlendArgs BlendIn;
				BlendIn = Montage->GetBlendInArgs();
				// Instantly blend in if we swapped to skeletal mesh this frame to avoid pop
				BlendIn.BlendTime = AnimationData.bSwappedThisFrame ? 0.0f : BlendIn.BlendTime;

				AnimInstance->Montage_PlayWithBlendIn(Montage, BlendIn, 1.0f, EMontagePlayReturnType::MontageLength, MontageFragment->MontageInstance.GetPosition());
			}

			// Force an animation update if we swapped this frame to prevent t-posing
			if (AnimationData.bSwappedThisFrame)
			{
				if (USkeletalMeshComponent* OwningComp = AnimInstance->GetOwningComponent())
				{
					TArray<USkeletalMeshComponent*> MeshComps;

					// Tick main component and all attached parts to avoid a frame of t-posing
					// We have to refresh bone transforms too because this can happen after the render state has been updated					

					OwningComp->TickAnimation(0.0f, false);
					OwningComp->RefreshBoneTransforms();

					Actor->GetComponents<USkeletalMeshComponent>(MeshComps, true);
					MeshComps.Remove(OwningComp);
					for (USkeletalMeshComponent* MeshComp : MeshComps)
					{
						MeshComp->TickAnimation(0.0f, false);
						MeshComp->RefreshBoneTransforms();
					}
				}
			}
		}
	}
}

void UMassProcessor_Animation::ConfigureQueries()
{
	AnimationEntityQuery_Conditional.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	AnimationEntityQuery_Conditional.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	AnimationEntityQuery_Conditional.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	AnimationEntityQuery_Conditional.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	AnimationEntityQuery_Conditional.AddRequirement<FMassVelocityFragment>(EMassFragmentAccess::ReadOnly);
	AnimationEntityQuery_Conditional.AddRequirement<FMassLookAtFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	AnimationEntityQuery_Conditional.AddRequirement<FMassMoveTargetFragment>(EMassFragmentAccess::ReadOnly, EMassFragmentPresence::Optional);
	AnimationEntityQuery_Conditional.AddRequirement<FCrowdAnimationFragment>(EMassFragmentAccess::ReadWrite);
	AnimationEntityQuery_Conditional.AddRequirement<FMassMontageFragment>(EMassFragmentAccess::ReadWrite, EMassFragmentPresence::Optional);
	AnimationEntityQuery_Conditional.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadOnly);
	AnimationEntityQuery_Conditional.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	AnimationEntityQuery_Conditional.RequireMutatingWorldAccess();

	MontageEntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadWrite);
	MontageEntityQuery.AddRequirement<FMassRepresentationFragment>(EMassFragmentAccess::ReadOnly);
	MontageEntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	MontageEntityQuery.AddRequirement<FMassActorFragment>(EMassFragmentAccess::ReadWrite);
	MontageEntityQuery.AddRequirement<FCrowdAnimationFragment>(EMassFragmentAccess::ReadOnly);
	MontageEntityQuery.AddRequirement<FMassMontageFragment>(EMassFragmentAccess::ReadWrite);
	MontageEntityQuery.AddChunkRequirement<FMassVisualizationChunkFragment>(EMassFragmentAccess::ReadOnly);
	MontageEntityQuery.RequireMutatingWorldAccess();

	MontageEntityQuery_Conditional = MontageEntityQuery;
	MontageEntityQuery_Conditional.SetChunkFilter(&FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk);
	MontageEntityQuery_Conditional.RequireMutatingWorldAccess();
}

void UMassProcessor_Animation::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);

	World = Owner.GetWorld();
	check(World);
}

void UMassProcessor_Animation::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	check(World);

	QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_Run);

	const float GlobalTime = World->GetTimeSeconds();

	TArray<FMassEntityHandle, TInlineAllocator<32>> ActorEntities;
	
	{
		QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_UpdateMontage);
		MontageEntityQuery.ForEachEntityChunk(EntityManager, Context, [this, GlobalTime, &ActorEntities, &EntityManager](FMassExecutionContext& Context)
			{
				const int32 NumEntities = Context.GetNumEntities();
				TArrayView<FMassMontageFragment> MontageDataList = Context.GetMutableFragmentView<FMassMontageFragment>();
				if (!FMassVisualizationChunkFragment::AreAnyEntitiesVisibleInChunk(Context))
				{
					for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
					{
						// If we are not updating animation, we still need to accumulate skipped time to fixup animation on the next update.
						FMassMontageFragment& MontageData = MontageDataList[EntityIdx];

						MontageData.SkippedTime += Context.GetDeltaTimeSeconds();
					}
				}
				else
				{
					TConstArrayView<FMassRepresentationFragment> VisualizationList = Context.GetFragmentView<FMassRepresentationFragment>();
					for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
					{
						const FMassRepresentationFragment& Visualization = VisualizationList[EntityIdx];
						if (Visualization.CurrentRepresentation == EMassRepresentationType::None)
						{
							continue;
						}

						FMassMontageFragment& MontageFragment = MontageDataList[EntityIdx];

						const float MontagePositionPreAdvance = MontageFragment.MontageInstance.GetPosition();
						const float MontageLength = MontageFragment.MontageInstance.GetLength();
						float AdjustedDeltaTime = Context.GetDeltaTimeSeconds() + MontageFragment.SkippedTime;
						const float AdjustedPositionPostAdvance = MontagePositionPreAdvance + AdjustedDeltaTime;
						if (AdjustedPositionPostAdvance > MontageLength)
						{
							// If we've skipped over the remaining duration of the montage clear our fragment
							MontageFragment.Clear();
							Context.Defer().PushCommand<FMassCommandRemoveFragments<FMassMontageFragment>>(Context.GetEntity(EntityIdx));
						}
						else
						{
							MontageFragment.RootMotionParams.Clear();

							UE::VertexAnimation::FLightWeightMontageExtractionSettings ExtractionSettings;

							if (MontageFragment.InteractionRequest.AlignmentTrack != NAME_None && MontageFragment.SkippedTime > 0.0f)
							{
								const UContextualAnimSceneAsset* ContextualAnimAsset = MontageFragment.InteractionRequest.ContextualAnimAsset.Get();
								if (ContextualAnimAsset)
								{
									FContextualAnimQueryResult& QueryResult = MontageFragment.InteractionRequest.QueryResult;

									const FContextualAnimTrack* AnimData = ContextualAnimAsset->GetAnimTrack(0, QueryResult.AnimSetIdx, MontageFragment.InteractionRequest.InteractorRole);

									const float WarpDuration = AnimData ? AnimData->GetSyncTimeForWarpSection(0) : 0.f;

									const float WarpDurationSkippedDelta = WarpDuration - MontagePositionPreAdvance;
									if (MontageFragment.SkippedTime > WarpDurationSkippedDelta)
									{
										// If we skipped past the warp, don't extract root motion for that portion, because we want to snap to the warp target before applying root motion.
										ExtractionSettings.bExtractRootMotion = false;
										MontageFragment.MontageInstance.Advance(WarpDurationSkippedDelta, GlobalTime, MontageFragment.RootMotionParams, ExtractionSettings);

										// Remaining time delta should not include warp duration we skipped
										AdjustedDeltaTime -= WarpDurationSkippedDelta;
									}
								}
							}

							ExtractionSettings.bExtractRootMotion = true;
							MontageFragment.MontageInstance.Advance(AdjustedDeltaTime, GlobalTime, MontageFragment.RootMotionParams, ExtractionSettings);
						}
					}
				}
			});
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_UpdateAnimationFragmentData);
		AnimationEntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [this, GlobalTime, &ActorEntities, &EntityManager](FMassExecutionContext& Context)
			{
				UMassProcessor_Animation::UpdateAnimationFragmentData(EntityManager, Context, GlobalTime, ActorEntities);
			});
	}
	{
		QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_UpdateVertexAnimationState);
		AnimationEntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [this, GlobalTime, &EntityManager](FMassExecutionContext& Context)
			{
				UMassProcessor_Animation::UpdateVertexAnimationState(EntityManager, Context, GlobalTime);
			});
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_ConsumeRootMotion);
		MontageEntityQuery_Conditional.ForEachEntityChunk(EntityManager, Context, [this, &EntityManager](FMassExecutionContext& Context)
			{
				TArrayView<FTransformFragment> TransformList = Context.GetMutableFragmentView<FTransformFragment>();
				TConstArrayView<FMassRepresentationFragment> VisualizationList = Context.GetFragmentView<FMassRepresentationFragment>();
				TConstArrayView<FCrowdAnimationFragment> AnimationDataList = Context.GetFragmentView<FCrowdAnimationFragment>();
				TArrayView<FMassMontageFragment> MontageDataList = Context.GetMutableFragmentView<FMassMontageFragment>();

				const int32 NumEntities = Context.GetNumEntities();
				for (int32 EntityIdx = 0; EntityIdx < NumEntities; EntityIdx++)
				{
					const FMassRepresentationFragment& Visualization = VisualizationList[EntityIdx];
					if (Visualization.CurrentRepresentation == EMassRepresentationType::None)
					{
						continue;
					}

					const FCrowdAnimationFragment& AnimationData = AnimationDataList[EntityIdx];
					FTransformFragment& TransformFragment = TransformList[EntityIdx];
					FMassMontageFragment& MontageFragment = MontageDataList[EntityIdx];

					const UContextualAnimSceneAsset* ContextualAnimAsset = MontageFragment.InteractionRequest.ContextualAnimAsset.Get();
					if (MontageFragment.InteractionRequest.AlignmentTrack != NAME_None && MontageFragment.MontageInstance.IsValid() && ContextualAnimAsset)
					{
						FContextualAnimQueryResult& QueryResult = MontageFragment.InteractionRequest.QueryResult;

						const FContextualAnimTrack* AnimData = ContextualAnimAsset->GetAnimTrack(0, QueryResult.AnimSetIdx, MontageFragment.InteractionRequest.InteractorRole);

						const float WarpDuration = AnimData ? AnimData->GetSyncTimeForWarpSection(0) : 0.f;
						const float MontagePosition = MontageFragment.MontageInstance.GetPosition();

						FVector TargetLocation;
						FQuat TargetRotation;

						const FTransform& PrevTransform = TransformFragment.GetTransform();
						FQuat PrevRot = PrevTransform.GetRotation();
						FVector PrevLoc = PrevTransform.GetTranslation();

						// Simple lerp towards interaction sync point
						UE::CrowdInteractionAnim::FMotionWarpingScratch& Scratch = MontageFragment.MotionWarpingScratch;

						if (MontagePosition < WarpDuration)
						{
							if (Scratch.Duration < 0.0f)
							{
								Scratch.InitialLocation = PrevLoc;
								Scratch.InitialRotation = PrevRot;
								Scratch.TimeRemaining = WarpDuration - MontagePosition;
								Scratch.Duration = Scratch.TimeRemaining;
							}
							Scratch.TimeRemaining -= Context.GetDeltaTimeSeconds();

							const FTransform& SyncTransform = QueryResult.SyncTransform;

							const float Alpha = FMath::Clamp((Scratch.Duration - Scratch.TimeRemaining) / Scratch.Duration, 0.0f, 1.0f);
							TargetLocation = FMath::Lerp(Scratch.InitialLocation, SyncTransform.GetLocation(), Alpha);

							TargetRotation = FQuat::Slerp(Scratch.InitialRotation, SyncTransform.GetRotation(), FMath::Pow(Alpha, 1.5f));
							TargetRotation.Normalize();
						}
						// Apply root motion
						else
						{
							if (MontagePosition - MontageFragment.SkippedTime < WarpDuration)
							{
								// If we skipped past the warp duration, snap to our sync point before applying root motion
								const FTransform& SyncTransform = QueryResult.SyncTransform;
								PrevLoc = SyncTransform.GetLocation();
								PrevRot = SyncTransform.GetRotation();
							}

							Scratch.Duration = -1.0f;

							const FTransform& RootMotionTransform = MontageFragment.RootMotionParams.GetRootMotionTransform();

							const FQuat ComponentRot = PrevRot * FQuat(FVector::UpVector, -90.0f);
							TargetLocation = PrevLoc + ComponentRot.RotateVector(RootMotionTransform.GetTranslation());
							TargetRotation = RootMotionTransform.GetRotation() * PrevRot;
						}

						MontageFragment.SkippedTime = 0.0f;
						TransformFragment.GetMutableTransform().SetLocation(TargetLocation);
						TransformFragment.GetMutableTransform().SetRotation(TargetRotation);
					}
				}
			});
	}

	{
		QUICK_SCOPE_CYCLE_COUNTER(UMassProcessor_Animation_UpdateSkeletalAnimation);
		// Pull out UAnimToTextureDataAsset from the inner loop to avoid the resolve cost, which is extremely high in PIE.
		UMassProcessor_Animation::UpdateSkeletalAnimation(EntityManager, GlobalTime, MakeArrayView(ActorEntities));
	}
}

class UAnimInstance* UMassProcessor_Animation::GetAnimInstanceFromActor(const AActor* Actor)
{
	const USkeletalMeshComponent* SkeletalMeshComponent = nullptr;
	if (const ACharacter* Character = Cast<ACharacter>(Actor))
	{
		SkeletalMeshComponent = Character->GetMesh();
	}
	else if (Actor)
	{
		SkeletalMeshComponent = Actor->FindComponentByClass<USkeletalMeshComponent>();
	}

	if (SkeletalMeshComponent)
	{
		return SkeletalMeshComponent->GetAnimInstance();
	}

	return nullptr;
}
