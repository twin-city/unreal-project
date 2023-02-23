// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassObserverProcessor.h"
#include "MassRepresentationTypes.h"
#include "MassCrowdAnimationProcessor.generated.h"

class UAnimToTextureDataAsset;
struct FMassActorFragment;

UCLASS()
class CITYSAMPLEMASSCROWD_API UMassFragmentInitializer_Animation : public UMassObserverProcessor
{
	GENERATED_BODY()

public:
	UMassFragmentInitializer_Animation();

protected:
	virtual void ConfigureQueries() override;
	virtual void Initialize(UObject& Owner) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	UPROPERTY()
	UWorld* World = nullptr;

	FMassEntityQuery EntityQuery;
};

UCLASS()
class CITYSAMPLEMASSCROWD_API UMassProcessor_Animation : public UMassProcessor 
{
	GENERATED_BODY()

public:
	UMassProcessor_Animation();

	UPROPERTY(EditAnywhere, Category="Animation", meta=(ClampMin=0.0, UIMin=0.0))
	float MoveThresholdSq = 750.0f;

private:
	void UpdateAnimationFragmentData(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float GlobalTime, TArray<FMassEntityHandle, TInlineAllocator<32>>& ActorEntities);
	void UpdateVertexAnimationState(FMassEntityManager& EntityManager, FMassExecutionContext& Context, float GlobalTime);
	void UpdateSkeletalAnimation(FMassEntityManager& EntityManager, float GlobalTime, TArrayView<FMassEntityHandle> ActorEntities);

protected:

	/** Configure the owned FMassEntityQuery instances to express processor's requirements */
	virtual void ConfigureQueries() override;
	virtual void Initialize(UObject& Owner) override;
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

	static class UAnimInstance* GetAnimInstanceFromActor(const class AActor* Actor);

	UPROPERTY(Transient)
	UWorld* World = nullptr;

	FMassEntityQuery AnimationEntityQuery_Conditional;
	FMassEntityQuery MontageEntityQuery;
	FMassEntityQuery MontageEntityQuery_Conditional;
};