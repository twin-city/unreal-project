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
struct FCoordinates: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float	x;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float	y;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float	z;
};

USTRUCT(BlueprintType)
struct FLinks: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int						district_id;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

USTRUCT(BlueprintType)
struct FNeighborhood: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int				id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<int>		neighbors;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FLinks>	neighbor_links;
};

/**************************/
/*		BUILDINGS		  */
/**************************/

USTRUCT(BlueprintType)
struct FBuilding: public FTableRowBase
{
	GENERATED_BODY()

	public:

		// UPROPERTY(EditAnywhere, BlueprintReadOnly)
		// FString					type;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		TArray<FCoordinates>	coordinates;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					id;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float					construction_year;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					mat_mur;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		FString					mat_toit;

		UPROPERTY(EditAnywhere, BlueprintReadOnly)
		float					height;
};

/**************************/
/*	    BUS SHELTERS  	  */
/**************************/

USTRUCT(BlueprintType)
struct FBusShelter: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

/**************************/
/*		BOLLARDS		  */
/**************************/

USTRUCT(BlueprintType)
struct FBollard: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

/**************************/
/*		LIGHT			  */
/**************************/

USTRUCT(BlueprintType)
struct FMyLight: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					support;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float					height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

/**************************/
/*		TREE			  */
/**************************/

USTRUCT(BlueprintType)
struct FTree: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int						circumference;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int						height;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

/**************************/
/*		ROAD			  */
/**************************/

USTRUCT(BlueprintType)
struct FRoad: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					street_name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int						width;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					traffic_direction;
};

USTRUCT(BlueprintType)
struct FGeom: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString					type;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCoordinates>	coordinates;
};

USTRUCT(BlueprintType)
struct FDistrict: public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FString				name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int					district;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int					id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FGeom				geom;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<int>			neighbors;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FBusShelter>	bus_shelters;
};

/****************************************************/

USTRUCT(BlueprintType)
struct TWINCITY_API FMyDataTable : public FTableRowBase
{
	GENERATED_BODY()

	public:
		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		TArray<UDataTable *>	districts;

		UPROPERTY(EditAnywhere, BlueprintReadWrite)
		UDataTable				*neighborhood;

};