// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleMassCrowdRepresentationActorManagement.h"
#include "IMassCrowdActor.h"
#include "MassEntityManager.h"

#include "MassCrowdSpawnerSubsystem.h"
#include "Components/CapsuleComponent.h"

EMassActorSpawnRequestAction UCitySampleMassCrowdRepresentationActorManagement::OnPostActorSpawn(const FMassActorSpawnRequestHandle& SpawnRequestHandle, FConstStructView SpawnRequest, FMassEntityManager* EntitySubsystem) const
{
	check(EntitySubsystem);

	const EMassActorSpawnRequestAction Result = Super::OnPostActorSpawn(SpawnRequestHandle, SpawnRequest, EntitySubsystem);
	
	const FMassActorSpawnRequest& MassActorSpawnRequest = SpawnRequest.Get<FMassActorSpawnRequest>();
	checkf(MassActorSpawnRequest.SpawnedActor, TEXT("Expecting valid spawned actor"));

	if (IMassCrowdActorInterface* MassCrowdActor = Cast<IMassCrowdActorInterface>(MassActorSpawnRequest.SpawnedActor))
	{
		MassCrowdActor->OnGetOrSpawn(EntitySubsystem, MassActorSpawnRequest.MassAgent);
	}

	return Result;
}

void UCitySampleMassCrowdRepresentationActorManagement::TeleportActor(const FTransform& Transform, AActor& Actor, FMassCommandBuffer& CommandBuffer) const
{
	FTransform RootTransform = Transform;

	if (const UCapsuleComponent* CapsuleComp = Actor.FindComponentByClass<UCapsuleComponent>())
	{
		const FVector HalfHeight(0.0f, 0.0f, CapsuleComp->GetScaledCapsuleHalfHeight());
		RootTransform.AddToTranslation(HalfHeight);

		const FVector RootLocation = RootTransform.GetLocation();
		const FVector SweepOffset(0.0f, 0.0f, 20.0f);
		const FVector Start = RootLocation + SweepOffset;
		const FVector End = RootLocation - SweepOffset;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(&Actor);
		FHitResult OutHit;
		if (Actor.GetWorld()->SweepSingleByChannel(OutHit, Start, End, Transform.GetRotation(), CapsuleComp->GetCollisionObjectType(), CapsuleComp->GetCollisionShape(), Params))
		{
			if (IMassCrowdActorInterface* MassCrowdActor = Cast<IMassCrowdActorInterface>(&Actor))
			{
				MassCrowdActor->SetAdditionalMeshOffset(RootTransform.GetLocation().Z - OutHit.Location.Z);
			}
			RootTransform.SetLocation(OutHit.Location);
		}
	}
	// Skip Super(UMassCrowdRepresentationActorManagement) because we already done that work here and we needed to know the offset to send to the feet IK logic
	UMassRepresentationActorManagement::TeleportActor(RootTransform, Actor, CommandBuffer);
}
