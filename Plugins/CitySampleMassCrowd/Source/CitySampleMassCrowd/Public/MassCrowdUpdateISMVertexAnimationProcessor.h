// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassUpdateISMProcessor.h"

#include "MassCrowdUpdateISMVertexAnimationProcessor.generated.h"

struct FMassInstancedStaticMeshInfo;
struct FCrowdAnimationFragment;

UCLASS()
class CITYSAMPLEMASSCROWD_API UMassCrowdUpdateISMVertexAnimationProcessor : public UMassUpdateISMProcessor
{
	GENERATED_BODY()
public:
	UMassCrowdUpdateISMVertexAnimationProcessor();

	static void UpdateISMVertexAnimation(FMassInstancedStaticMeshInfo& ISMInfo, FCrowdAnimationFragment& AnimationData, const float LODSignificance, const float PrevLODSignificance, const int32 NumFloatsToPad = 0);

protected:

	/** Configure the owned FMassEntityQuery instances to express processor's requirements */
	virtual void ConfigureQueries() override;

	/**
	 * Execution method for this processor
	 * @param EntitySubsystem is the system to execute the lambdas on each entity chunk
	 * @param Context is the execution context to be passed when executing the lambdas */
	virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;
};