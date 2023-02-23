// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ContextualAnimSceneAsset.h"
#include "MassCommonFragments.h"
#include "LightweightMontageInstance.h"
#include "MassCrowdAnimationTypes.generated.h"

class UAnimInstance;
class UAnimToTextureDataAsset;

namespace UE::CrowdInteractionAnim
{
	struct FRequest
	{
		TWeakObjectPtr<class UContextualAnimSceneAsset> ContextualAnimAsset = nullptr;
		FContextualAnimQueryResult QueryResult = FContextualAnimQueryResult();
		FName InteractorRole;
		FName AlignmentTrack;
	};

	struct FMotionWarpingScratch
	{
		float TimeRemaining = -1.0f;
		float Duration = -1.0f;
		FVector InitialLocation = FVector::ZeroVector;
		FQuat InitialRotation = FQuat::Identity;
	};
} // namespace UE::CrowdInteractionAnim

USTRUCT()
struct CITYSAMPLEMASSCROWD_API FMassMontageFragment : public FMassFragment
{
	GENERATED_BODY()

	UE::VertexAnimation::FLightweightMontageInstance MontageInstance = UE::VertexAnimation::FLightweightMontageInstance();
	UE::CrowdInteractionAnim::FRequest InteractionRequest = UE::CrowdInteractionAnim::FRequest();
	UE::CrowdInteractionAnim::FMotionWarpingScratch MotionWarpingScratch = UE::CrowdInteractionAnim::FMotionWarpingScratch();
	FRootMotionMovementParams RootMotionParams = FRootMotionMovementParams();
	float SkippedTime = 0.0f;
	
	void Request(const UE::CrowdInteractionAnim::FRequest& InRequest);
	void Clear();
};

USTRUCT()
struct CITYSAMPLEMASSCROWD_API FCrowdAnimationFragment : public FMassFragment
{
	GENERATED_BODY()

	TWeakObjectPtr<UAnimToTextureDataAsset> AnimToTextureData;
	float GlobalStartTime = 0.0f;
	float PlayRate = 1.0f;
	int32 AnimationStateIndex = 0;
	bool bSwappedThisFrame = false;
};