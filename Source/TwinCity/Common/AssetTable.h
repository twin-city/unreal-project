// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AssetTable.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct TWINCITY_API FAssetTable : public FTableRowBase
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		coordActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		wallActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		roadActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		bollardActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		treeActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		lightActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		busShelterActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>		undergroundStationActor;
};
