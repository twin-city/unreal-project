// Copyright Epic Games, Inc. All Rights Reserved.
#include "Animation/MassCrowdAnimInstance.h"

#include "Animation/MassCrowdAnimationSettings.h"
#include "Engine/Canvas.h"

UMassCrowdAnimInstance::UMassCrowdAnimInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMassCrowdAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
}

void UMassCrowdAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	FMassCharacterProximity_UpdateContext UpdateContext;
	UpdateContext.DeltaSeconds = DeltaSeconds;
	UpdateContext.MeshTransform = GetOwningComponent()->GetComponentTransform();
	PlayerProximityData.Update(UpdateContext);

	// Don't queue a new trace if we haven't moved in a while. A previously queued trace will still complete.
	if (GetOwningActor()->GetVelocity().IsZero() && NoVelocityTimer > 0.0f)
	{
		NoVelocityTimer -= DeltaSeconds;
	}
	else
	{
		NoVelocityTimer = 0.3f;
	}

	if (NoVelocityTimer > 0.0f)
	{
		QueueFootTraces();
	}

	UpdateFootTracesState();
}

void UMassCrowdAnimInstance::DisplayDebugInstance(FDisplayDebugManager& DisplayDebugManager, float& Indent)
{
	Super::DisplayDebugInstance(DisplayDebugManager, Indent);

	if (const AActor* OwningActor = GetOwningActor())
	{
		TInlineComponentArray<const USkeletalMeshComponent*, 16> SkeletalMeshComponents(OwningActor);
		for (const USkeletalMeshComponent* SkelComp : SkeletalMeshComponents)
		{
			FString DebugText = FString::Printf(TEXT("SkeletalMeshComponent:(%s), SkeletalMesh: (%s)"), *GetNameSafe(SkelComp), *GetNameSafe(SkelComp->GetSkeletalMeshAsset()));
			DisplayDebugManager.DrawString(DebugText, Indent);
		}
	}
}

void UMassCrowdAnimInstance::NativePostEvaluateAnimation()
{
	Super::NativePostEvaluateAnimation();
	
	// Consume the value in post eval
	MassCrowdAnimInstanceData.bSwappedThisFrame = false;
}

void UMassCrowdAnimInstance::QueueFootTraces()
{
	if (!bFootTracesRequested)
	{
		bFootTracesRequested = true;
		if (const UMassCrowdAnimationSettings* Settings = UMassCrowdAnimationSettings::Get())
		{
			const int32 NumLODFrequencies = Settings->CrowdAnimFootLODTraceFrequencyPerLOD.Num();
			if (NumLODFrequencies > 0)
			{	
				int32 Frequency;
				if (GetOwningComponent()->GetNumLODs() <= 1)
				{	
					const FVector2D RangeMin = FVector2D(0.0f, 2.0f);
					const FVector2D RangeMax = FVector2D(Settings->CrowdAnimFootLODTraceFrequencyPerLOD[0], Settings->CrowdAnimFootLODTraceFrequencyPerLOD[NumLODFrequencies - 1]);
					Frequency = FMath::RoundToInt(FMath::GetMappedRangeValueClamped(RangeMin, RangeMax, MassCrowdAnimInstanceData.Significance));
				}
				else
				{
					const int32 LODFrequencyIndexToUse = FMath::Min(GetOwningComponent()->GetPredictedLODLevel(), NumLODFrequencies - 1);
					Frequency = Settings->CrowdAnimFootLODTraceFrequencyPerLOD[LODFrequencyIndexToUse];
				}

				Frequency = FMath::Max(0, Frequency);

				FramesUntilFootTraces = int32(UMassCrowdAnimInstance::DoFootTracesFrameOffset % (1 + Frequency));
				UMassCrowdAnimInstance::DoFootTracesFrameOffset++;
				return;
			}
		}
	}
}

void UMassCrowdAnimInstance::UpdateFootTracesState()
{
	bDoFootTraces = false;
	if (bFootTracesRequested)
	{
		FramesUntilFootTraces--;

		if (FramesUntilFootTraces <= 0)
		{
			bFootTracesRequested = false;
			bDoFootTraces = true;
		}
	}
}

int32 UMassCrowdAnimInstance::DoFootTracesFrameOffset = 0;
