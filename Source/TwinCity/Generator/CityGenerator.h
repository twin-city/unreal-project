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

		// UPROPERTY(EditAnywhere, Category = Location)
		// FVector NewLocation = FVector::ZeroVector;

		// UPROPERTY(EditAnywhere, Category = Location)
		// FQuat NewRotation;

		// UPROPERTY(EditAnywhere)
    	// FVector	offsetf;

		UFUNCTION(BlueprintCallable, Category = Generator)
		void _generateFromDT(UDataTable	*districtTable);

	private:

		const float				defaultValue = 1.0f;
		const float 			defaultHeight = 50.0f;
		ASceneDistrict			*districtActor;
		FAssetTable				*assets;
		FDistrict 				*district;
		// FVector					offset = FVector(651348.2364000008, -6861734.417099999, 0);

		/************************************************/
		/*                 COMMON						*/
		/************************************************/
		
		void		_generateDistrict();
		FDistrict 	*_newDistrictActor(UDataTable *newDistrict);
		FRotator	_getNewRotation(FVector const &v1, FVector const &v2);
		FVector		_getMeanVector(FVector const &v1, FVector const &v2);
		template	<class T>
		FVector		_getCoordLocation(int const i, T const obj);
		FString		_missingData() const;
		bool		_checkAvailableData() const;
		template	<class T>
		void		_setNewActor(T const obj, float depth, TSubclassOf<AActor> const &actorToSpawn);
		void		_drawDistrictsBoundaries(FGeom const &geom, TSubclassOf<AActor> const &actorToSpawn);
		bool		_isInDistrict(FVector pos);

		/************************************************/
		/*               BUILDINGS						*/
		/************************************************/

		void		_generateBuildings(TArray<FBuilding> const &buildings);
		void		_generateWalls(FBuilding const *buildings);
		void		_spawnWall(FVector const &v1, FVector const &v2, const float depth);

		/************************************************/
		/*                 ROADS						*/
		/************************************************/

		void		_generateRoads(TArray<FRoad> const &roads);
		void		_spawnRoad(FVector const &v1, FVector const &v2, float const depth);

		/************************************************/
		/*               TREES							*/
		/************************************************/
		
		void		_generateTrees(TArray<FTree> const &trees);

		/************************************************/
		/*               LIGHTS							*/
		/************************************************/
		
		void		_generateLights(TArray<FMyLight> const &lights);

		/************************************************/
		/*                 BOLLARDS						*/
		/************************************************/

		void		_generateBollards(TArray<FBollard> const &bollards);

};
