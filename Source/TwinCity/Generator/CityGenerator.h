// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "../Common/AssetTable.h" 
#include "../Common/MyDataTable.h"
#include "CoreMinimal.h"
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
		UDataTable	*assetTable;

		UPROPERTY(EditAnywhere)
		float 	scale = 100.0f;

		UPROPERTY(EditAnywhere)
		FName	myChoice = "";

		UPROPERTY(EditAnywhere, Category = Location)
		FVector NewLocation = FVector::ZeroVector;

		UPROPERTY(EditAnywhere, Category = Location)
		FQuat NewRotation;

		UFUNCTION(BlueprintCallable, Category = Generator)
		void _generateFromDT();

	private:

		const float	defaultValue = 1.0f;
		const float defaultHeight = 50.0f;
		
		/************************************************/
		/*                 COMMON						*/
		/************************************************/
		
		void		_generateDistrict(FDistrict	*district, FAssetTable *assets);
		FRotator	_getNewRotation(FVector const &v1, FVector const &v2);
		FVector		_getMeanVector(FVector const &v1, FVector const &v2);
		template	<class T>
		FVector		_getCoordLocation(int const i, T const obj);
		FString		_missingData(FDistrict const *district) const;
		bool		_checkAvailableData(FDistrict const *district) const;
		template	<class T>
		void		_setNewActor(T const obj, float depth, TSubclassOf<AActor> const &actorToSpawn, AActor* district);
		void		_drawDistrictsBoundaries(FGeom const &geom, AActor* district, TSubclassOf<AActor> const &actorToSpawn);

		/************************************************/
		/*               BUILDINGS						*/
		/************************************************/

		void		_generateBuildings(TArray<FBuilding> const &buildings, AActor* district, FAssetTable *assets);
		void		_generateWalls(FBuilding const *buildings, AActor* district);
		void		_spawnWall(FVector const &v1, FVector const &v2, const float depth, AActor* district, FAssetTable *assets);

		/************************************************/
		/*                 ROADS						*/
		/************************************************/

		void		_generateRoads(TArray<FRoad> const &roads, AActor* district, FAssetTable *assets);
		void		_spawnRoad(FVector const &v1, FVector const &v2, float const depth, AActor* district, FAssetTable *assets);

		/************************************************/
		/*               TREES							*/
		/************************************************/
		
		void		_generateTrees(TArray<FTree> const &trees, AActor* district, FAssetTable *assets);

		/************************************************/
		/*               LIGHTS							*/
		/************************************************/
		
		void		_generateLights(TArray<FMyLight> const &lights, AActor* district, FAssetTable *assets);

		/************************************************/
		/*                 BOLLARDS						*/
		/************************************************/

		void		_generateBollards(TArray<FBollard> const &bollards, AActor* district, FAssetTable *assets);
};
