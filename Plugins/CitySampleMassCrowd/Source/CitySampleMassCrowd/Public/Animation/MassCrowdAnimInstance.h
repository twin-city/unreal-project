// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimInstance.h"
#include "MassCharacterStopPrediction_AnimInput.h"
#include "MassCharacterProximity_AnimInput.h"
#include "MassCharacterMovementInfo_AnimInput.h"
#include "MassCrowdAnimInstance.generated.h"

class UAnimSequence;

USTRUCT(BlueprintType)
struct FMassCrowdAnimInstanceData
{
	GENERATED_BODY()

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	UAnimSequence* FarLODAnimSequence = nullptr;
	
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FTransform MassEntityTransform;

	// In local/component space
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FVector LookAtDirection = FVector::ForwardVector;
	
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	float FarLODPlaybackStartTime = 0.0f;

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	float Significance = 0.0f;

	// Default to true to assume we always swapped on init
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	bool bSwappedThisFrame = true;
};

UCLASS(transient)
class CITYSAMPLEMASSCROWD_API UMassCrowdAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	void SetMassAnimInstanceData(const FMassCrowdAnimInstanceData& InInstanceData)
	{
		MassCrowdAnimInstanceData = InInstanceData;
	}

	UMassCrowdAnimInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual void NativePostEvaluateAnimation() override;

	virtual void DisplayDebugInstance(FDisplayDebugManager& DisplayDebugManager, float& Indent);

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FMassCharacterProximity_AnimInput PlayerProximityData;

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FMassCharacterStopPrediction_AnimInput StopPredictionData;

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FMassCharacterMovementInfo_AnimInput MassMovementInfo;

	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	bool bDoFootTraces = false;

	const FTransform& GetMassEntityTransform() const {return MassCrowdAnimInstanceData.MassEntityTransform;}
	bool MassSwappedThisFrame() const {return MassCrowdAnimInstanceData.bSwappedThisFrame;}

protected:
	UPROPERTY(transient, EditAnywhere, BlueprintReadOnly, Category = MassCrowd)
	FMassCrowdAnimInstanceData MassCrowdAnimInstanceData;
private:
	void QueueFootTraces();
	void UpdateFootTracesState();

	float NoVelocityTimer = 0.0f;
	int32 FramesUntilFootTraces = 0;
	bool bFootTracesRequested = false;
	static int32 DoFootTracesFrameOffset;
};

