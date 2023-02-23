// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassCrowdRepresentationActorManagement.h"

#include "CitySampleMassCrowdRepresentationActorManagement.generated.h"

struct FMassEntityManager;
class UMassCrowdSpawnerSubsystem;

/**
* Overridden actor management fro forsty specific
*/
UCLASS(meta = (DisplayName = "CitySample Crowd Visualization"))
class CITYSAMPLEMASSCROWD_API UCitySampleMassCrowdRepresentationActorManagement : public UMassCrowdRepresentationActorManagement
{
	GENERATED_BODY()

	/**
	* Method that will be bound to a delegate used post-spawn to notify and let the requester configure the actor
	* @param SpawnRequestHandle the handle of the spawn request that was just spawned
	* @param SpawnRequest of the actor that just spawned
	* @param EntitySubsystem to use to retrieve the mass agent fragments
	* @return The action to take on the spawn request, either keep it there or remove it.
	*/
	virtual EMassActorSpawnRequestAction OnPostActorSpawn(const FMassActorSpawnRequestHandle& SpawnRequestHandle, FConstStructView SpawnRequest, FMassEntityManager* EntitySubsystem) const override;

	/**
	 * Teleports the actor at the specified transform by preserving its velocity and without collision.
	 * The destination will be adjusted to fit an existing capsule.
	 * @param Transform is the new actor's transform
	 * @param Actor is the actual actor to teleport
	 * @param CommandBuffer to queue up anything that is thread sensitive
	 */
	 virtual void TeleportActor(const FTransform& Transform, AActor& Actor, FMassCommandBuffer& CommandBuffer) const override;
};