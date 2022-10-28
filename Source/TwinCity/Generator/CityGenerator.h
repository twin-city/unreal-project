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
		UDataTable	*myBollards;

		UPROPERTY(EditAnywhere)
		UDataTable	*myBuildings;

		UPROPERTY(EditAnywhere)
		UDataTable	*myLights;

		UPROPERTY(EditAnywhere)
		UDataTable	*myRoads;

		UPROPERTY(EditAnywhere)
		UDataTable	*myTrees;

		UPROPERTY(EditAnywhere)
		FVector	offset = FVector::ZeroVector;

		UPROPERTY(EditAnywhere)
		float 	scale = 100.0f;

	private:

		const float	defaultValue = 1.0f;
		const float defaultHeight = 50.0f;
		
		/************************************************/
		/*                 COMMON						*/
		/************************************************/
		
		void		_generate();
		void		_spawnCoord(FVector const &location);
		FRotator	_getNewRotation(FVector const &v1, FVector const &v2);
		FVector		_getMeanVector(FVector const &v1, FVector const &v2);
		template	<class T>
		FVector		_getCoordLocation(int const i, int const j, T const *obj);
		FString		_missingData();
		bool		_checkAvailableData() const;
		void		_setNewActor(FCoordonnees coord, float depth, TSubclassOf<AActor> actorToSpawn);

		/************************************************/
		/*               BUILDINGS						*/
		/************************************************/

		void		_generateBuildings();
		void		_generateWalls(FGlobalBuildings const *buildings);
		void		_spawnWall(FVector const &v1, FVector const &v2, const float depth);

		/************************************************/
		/*                 ROADS						*/
		/************************************************/

		void		_generateRoads();
		void		_spawnRoad(FVector const &v1, FVector const &v2, const float height);

		/************************************************/
		/*               TREES							*/
		/************************************************/
		
		void		_generateTrees();

		/************************************************/
		/*               LIGHTS							*/
		/************************************************/
		
		void		_generateLights();

		/************************************************/
		/*                 BOLLARDS						*/
		/************************************************/

		void		_generateBollards();
};
