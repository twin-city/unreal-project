// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "MassSettings.h"
#include "MassCrowdAnimationSettings.generated.h"

UCLASS(config = Mass, defaultconfig, meta=(DisplayName="CitySample Mass Crowd Animation"))
class CITYSAMPLEMASSCROWD_API UMassCrowdAnimationSettings : public UMassModuleSettings
{
	GENERATED_BODY()

public:

	static const UMassCrowdAnimationSettings* Get()
	{
		return GetDefault<UMassCrowdAnimationSettings>();
	}

	UPROPERTY(EditAnywhere, config, Category = LOD);
	TArray<int32> CrowdAnimFootLODTraceFrequencyPerLOD = {5, 10, 15};

	UPROPERTY(EditAnywhere, config, Category = Anim);
	TArray<FName> CommonCrowdContextualAnimNames;

private:

	UFUNCTION()
	static TArray<FString> GetContextualAnimOptions()
	{
		TArray<FString> ContextualAnimNames;

		for (const FName& AnimName : UMassCrowdAnimationSettings::Get()->CommonCrowdContextualAnimNames)
		{
			if (AnimName != NAME_None)
			{
				ContextualAnimNames.Add(AnimName.ToString());
			}
		}

		return ContextualAnimNames;
	}
};
