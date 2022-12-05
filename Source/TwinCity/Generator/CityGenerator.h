// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Common/AssetTable.h" 
#include "Components/CapsuleComponent.h" 
#include "../Common/MyDataTable.h"
#include "CoreMinimal.h"
#include "../Common/SceneDistrict.h"
#include "GameFramework/Actor.h"
#include "CityGenerator.generated.h"


UCLASS()
class TWINCITY_API ACityGenerator : public AActor
{
	GENERATED_BODY()
	
	public:	
		ACityGenerator();

	protected:
		virtual void BeginPlay() override;

	public:	
		virtual void Tick(float DeltaTime) override;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UDataTable				*assetTable;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UDataTable				*cityTable;

		UPROPERTY(EditAnywhere)
		float 	scale = 100.0f;

		UPROPERTY(EditAnywhere)
		FName	choice = "";

		UFUNCTION(BlueprintCallable, Category = Generator)
		void _generateFromCityDT(UDataTable	*inputCityTable);

		UFUNCTION(BlueprintCallable, Category = Generator)
		void _generateNeighborhoodFromDT(FDistrict const &chosenDistrict);
	
		UFUNCTION(BlueprintCallable, Category = Generator)
		void _generateNeighborhoodFromCityDT(FMyDataTable const& inputCityTable, FDistrict const &chosenDistrict);

	private:

		const float				defaultValue = 1.0f;
		UPROPERTY()
		ASceneDistrict			*districtActor;
		FAssetTable				*assets;
		TArray<int>				neighborsId;

		/************************************************/
		/*                 COMMON						*/
		/************************************************/
		
		/*			CHECK VALUE			*/
		bool		_isInNeighborhood(FDistrict* district) const;
		FString		_missingData(FDistrict* district) const;
		bool		_checkAvailableData(FDistrict* district) const;

		/*			GET VALUE			*/
		FRotator	_getNewRotation(FVector const &v1, FVector const &v2);
		template	<class T>
		FVector		_getCoordLocation(int const i, T const obj);
		
		/*			SET VALUE			*/
		void		_generateDistrict(FDistrict* district);
		void		_drawDistrictsBoundaries(FGeom const &geom, TSubclassOf<AActor> const &actorToSpawn);
		template	<class T>
		void		_setNewActor(T const obj, float depth, TSubclassOf<AActor> const &actorToSpawn);
		template	<class T>
		void		_generateObjects(TArray<T> const &obj, TSubclassOf<AActor> const &actorToSpawn);
		void		_setDistrictActor(FDistrict *newDistrict);
		FDistrict	*_getChosenDistrict(FMyDataTable *city) const;

		/************************************************/
		/*               POPULATE						*/
		/************************************************/

		/*			BUILDINGS			*/
		void		_generateBuildings(TArray<FBuilding> const &buildings);
		void		_spawnWall(FVector const &v1, FVector const &v2, const float depth);

		/*			ROADS			*/
		void		_generateRoads(TArray<FRoad> const &roads);
		void		_spawnRoad(FVector const &v1, FVector const &v2, float const depth);

};
