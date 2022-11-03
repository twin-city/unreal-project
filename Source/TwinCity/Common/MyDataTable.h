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
struct FBuilding: public FTableRowBase
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
struct FDistrict: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FBuilding>	buildings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FRoad>		roads;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FMyLight>	lights;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FBollard>	bollards;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FTree>		trees;
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
