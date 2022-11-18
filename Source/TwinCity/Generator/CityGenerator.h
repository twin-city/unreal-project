// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

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

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	coordActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	wallActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	roadActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	bollardActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	treeActor;

		UPROPERTY(EditAnywhere)
		TSubclassOf<AActor>	lightActor;

		UPROPERTY(EditAnywhere)
		UDataTable	*myDistricts;

		UPROPERTY(EditAnywhere)
		float 	scale = 100.0f;

		UPROPERTY(EditAnywhere)
		FName	myChoice = "";

		UPROPERTY(EditAnywhere, Category = Location)
		FVector NewLocation = FVector::ZeroVector;

		UPROPERTY(EditAnywhere, Category = Location)
		FQuat NewRotation;

	private:

		const float	defaultValue = 1.0f;
		const float defaultHeight = 50.0f;
		
		/************************************************/
		/*                 COMMON						*/
		/************************************************/
		
		void		_generate(FDistrict	*district);
		FRotator	_getNewRotation(FVector const &v1, FVector const &v2);
		FVector		_getMeanVector(FVector const &v1, FVector const &v2);
		template	<class T>
		FVector		_getCoordLocation(int const i, T const obj);
		FString		_missingData(FDistrict const *district) const;
		bool		_checkAvailableData(FDistrict const *district) const;
		template	<class T>
		void		_setNewActor(T const obj, float depth, TSubclassOf<AActor> actorToSpawn);
		template	<class T>
		AActor		*_spawnObj(FVector const &location, T const objActor);
		void		_drawDistrictsBoundaries(FGeom geom);

		/************************************************/
		/*               BUILDINGS						*/
		/************************************************/

		void		_generateBuildings(TArray<FBuilding> buildings);
		void		_generateWalls(FBuilding const *buildings);
		void		_spawnWall(FVector const &v1, FVector const &v2, const float depth);

		/************************************************/
		/*                 ROADS						*/
		/************************************************/

		// void		_generateRoads(TArray<FRoad> roads);
		// void		_spawnRoad(FVector const &v1, FVector const &v2, const float height);

		/************************************************/
		/*               TREES							*/
		/************************************************/
		
		void		_generateTrees(TArray<FTree> trees);

		/************************************************/
		/*               LIGHTS							*/
		/************************************************/
		
		void		_generateLights(TArray<FMyLight> lights);

		/************************************************/
		/*                 BOLLARDS						*/
		/************************************************/

		void		_generateBollards(TArray<FBollard> bollards);
};
