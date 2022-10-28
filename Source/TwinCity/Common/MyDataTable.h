// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Containers/Array.h" 
#include "Engine/DataTable.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyDataTable.generated.h"

/**************************/
/*		COMMON			  */
/**************************/

USTRUCT(BlueprintType)
struct FCoordonnees: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float	x;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float	y;
};

/**************************/
/*		BUILDINGS		  */
/**************************/

USTRUCT(BlueprintType)
struct FWalls: public FTableRowBase
{
	GENERATED_BODY()
	
	public:
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					type;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FCoordonnees>	coordonnees;
};

USTRUCT(BlueprintType)
struct FBuildings: public FTableRowBase
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					id;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float					annee_construction;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					mat_mur;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					mat_toit;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float					hauteur;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					type;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FCoordonnees>	coordonnees;
};

USTRUCT(BlueprintType)
struct FGlobalBuildings: public FTableRowBase
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FBuildings>	data;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FWalls>		artifacts;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FWalls>		walls;
};

/**************************/
/*		BOLLARDS		  */
/**************************/

USTRUCT(BlueprintType)
struct FBollard: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString			type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCoordonnees	coordonnees;
};

USTRUCT(BlueprintType)
struct FGlobalBollards: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FBollard>	data;
};

/**************************/
/*		LIGHT			  */
/**************************/

USTRUCT(BlueprintType)
struct FMyLight: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString			support;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int				hauteur;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCoordonnees	coordonnees;
};

USTRUCT(BlueprintType)
struct FGlobalLights: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMyLight>	data;
};

/**************************/
/*		TREE			  */
/**************************/

USTRUCT(BlueprintType)
struct FTree: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int		hauteurenm;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString	libellefrancais;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FCoordonnees	coordonnees;
};

USTRUCT(BlueprintType)
struct FGlobalTree: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTree>	data;
};

/**************************/
/*		ROAD			  */
/**************************/

USTRUCT(BlueprintType)
struct FRoad: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString		nom_1_gauche;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int			largeur_de_chaussee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString		sens_de_circulation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordonnees>	coordonnees;
};

USTRUCT(BlueprintType)
struct FGlobalRoads: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FRoad>	data;
};

/****************************************************/

UCLASS()
class TWINCITY_API AMyDataTable : public AActor
{
	GENERATED_BODY()
	
public:	
	AMyDataTable();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

};
