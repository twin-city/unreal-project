// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MassEntityTypes.h"
#include "MassStateTreeTypes.h"
#include "MassContextualAnimTask.generated.h"

class UAnimMontage;

class UMassSignalSubsystem;
struct FMassMontageFragment; 
struct FTransformFragment;
struct FMassMoveTargetFragment;

USTRUCT()
struct FMassContextualAnimTaskInstanceData
{
	GENERATED_BODY()
	
	UPROPERTY(VisibleAnywhere, Category = Input, meta = (Optional))
	FMassEntityHandle TargetEntity;

	UPROPERTY(EditAnywhere, Category = Parameter)
	float Duration = 0.0f;

	UPROPERTY()
	float ComputedDuration = 0.0f;

	/** Accumulated time used to stop task if a montage is set */
	UPROPERTY()
	float Time = 0.f;
};

USTRUCT(meta = (DisplayName = "Mass Contextual Anim Task"))
struct FMassContextualAnimTask : public FMassStateTreeTaskBase
{
	GENERATED_BODY()

	using FInstanceDataType = FMassContextualAnimTaskInstanceData; 

	FMassContextualAnimTask();

protected:
	virtual bool Link(FStateTreeLinker& Linker) override;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

	TStateTreeExternalDataHandle<UMassSignalSubsystem> MassSignalSubsystemHandle;
	TStateTreeExternalDataHandle<FMassMontageFragment, EStateTreeExternalDataRequirement::Optional> MontageRequestHandle; 
	TStateTreeExternalDataHandle<FTransformFragment> TransformHandle;
	TStateTreeExternalDataHandle<FMassMoveTargetFragment> MoveTargetHandle;

	UPROPERTY(EditAnywhere, Category = Parameter)
	class UContextualAnimSceneAsset* ContextualAnimAsset = nullptr;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName AlignmentTrack;

	UPROPERTY(EditAnywhere, Category = Parameter)
	FName InteractorRole;

	UPROPERTY(EditAnywhere, Category = Parameter)
	UAnimMontage* FallbackMontage = nullptr;
};
