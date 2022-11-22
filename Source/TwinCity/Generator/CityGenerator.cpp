#include "CityGenerator.h"

/*
	TODO:
		- GET my player location
		- GENERATE() my district when the player is closed
*/

/************************************************/
/*                	BOLLARDS					*/
/************************************************/

void	ACityGenerator::_generateBollards(TArray<FBollard> bollards)
{
	for (int i = 0; i < bollards.Num(); i++)
	{
		_setNewActor(bollards[i], defaultValue, bollardActor);
	}
}

/************************************************/
/*                 ROADS						*/
/************************************************/

// void	ACityGenerator::_spawnRoad(FVector const &v1, FVector const &v2, float const depth)
// {
// 	FVector		meanVector = FMath::Lerp(v1, v2, 0.5f) * scale;
// 	meanVector.Z = defaultHeight;
// 	FRotator	newRotation = _getNewRotation(v1, v2);
// 	float		width = FVector::Dist(v1, v2);
// 	//	checker si possible de spawn et rescale en meme temps
// 	AActor		*myActor = GetWorld()->SpawnActor<AActor>(roadActor, meanVector, newRotation);
// 	FVector		newScale = FVector(width, depth, defaultValue);

// 	myActor->SetActorScale3D(newScale);
// }

// void	ACityGenerator::_generateRoads(TArray<FRoad> roads)
// {
// 	FVector	location, tmpLocation;

// 	for (int i = 0; i < roads.Num(); i++)
// 	{
// 		for (int j = 0; j < roads[i].coordinates.Num(); j++)
// 		{
// 			location = _getCoordLocation(j, roads[i]);
// 			// _spawnCoord(Location);
// 			if (j > 0)
// 				_spawnRoad(location, tmpLocation, roads[i].largeur_de_chaussee);
				
// 			tmpLocation = location;
// 		}
// 	}
// }

/************************************************/
/*               TREES							*/
/************************************************/

void	ACityGenerator::_generateTrees(TArray<FTree> trees)
{
	for (int i = 0; i < trees.Num(); i++)
	{
		_setNewActor(trees[i], defaultValue, treeActor);
	}
}

/************************************************/
/*               LIGHTS							*/
/************************************************/

void	ACityGenerator::_generateLights(TArray<FMyLight> lights)
{
	for (int i = 0; i < lights.Num(); i++)
	{
		_setNewActor(lights[i], lights[i].height, lightActor);
	}
}

/************************************************/
/*               BUILDINGS						*/
/************************************************/

void		ACityGenerator::_spawnWall(FVector const &v1, FVector const &v2, float const height)
{
	FVector		meanVector = FMath::Lerp(v1, v2, 0.5f) * scale;
	//	TODO: a rechecker
	meanVector.Z = height * scale / 2;
	FRotator	newRotation = _getNewRotation(v1, v2);
	float		width = FVector::Dist(v1, v2);
	AActor		*myActor = GetWorld()->SpawnActor<AActor>(wallActor, meanVector, newRotation);
	FVector		newScale = FVector(width, 1.f, height);	// scale -> defaultDepth

	myActor->SetActorScale3D(newScale);
}

void	ACityGenerator::_generateBuildings(TArray<FBuilding> buildings)
{
	FVector		location, tmpLocation;

	//	for each buildings
	for (int i = 0; i < buildings.Num(); i++)
	{
		//	for each vertices
		for (int j = 0; j < buildings[i].coordinates.Num(); j++)
		{
			location = _getCoordLocation(j, buildings[i]);
			// _spawnCoord(location * scale + buildings[i].height * FVector::UpVector * scale); // to change in prepare_data
			if (j > 0)
			{
				_spawnWall(location, tmpLocation, buildings[i].height);	// to change in prepare_data
			}
			tmpLocation = location;
		}
	}
}

/************************************************/
/*                 COMMON						*/
/************************************************/

template	<class T>
AActor	*ACityGenerator::_spawnObj(FVector const &location, T const objActor)
{
	return GetWorld()->SpawnActor<AActor>(objActor, location, FRotator::ZeroRotator);
}

void	ACityGenerator::_drawDistrictsBoundaries(FGeom geom)
{
	FVector location = FVector::ZeroVector;

	for (int i = 0; i < geom.coordinates.Num(); i++)
	{
		location = _getCoordLocation(i, geom) * scale;
		_spawnObj(location, coordActor);
	}
}

template	<class T>
void	ACityGenerator::_setNewActor(T const obj, float height, TSubclassOf<AActor> actorToSpawn)
{
	for (int i = 0; i < obj.coordinates.Num(); i++)
	{
		FVector	location = _getCoordLocation(i, obj) * scale;
		AActor	*myActor = _spawnObj(location, actorToSpawn);
		FVector	newScale = FVector(defaultValue, defaultValue, height);

		if (height == defaultValue)
			return;
		myActor->SetActorScale3D(newScale);
	}
}

bool	ACityGenerator::_checkAvailableData(FDistrict const *district) const
{
	//	TODO: masque binaire pour checker en fonction du quartier
	//	si les data necessaires existent bien
	if (!district)
		return false;
	else if (district->buildings.IsEmpty())
		return false;
	else if (district->bollards.IsEmpty())
		return false;
	else if (district->lights.IsEmpty())
		return false;
	return true;
}

ACityGenerator::ACityGenerator()
{
	PrimaryActorTick.bCanEverTick = true;

}

FRotator	ACityGenerator::_getNewRotation(FVector const &v1, FVector const &v2)
{
	FVector sub = v1 - v2;
	float	tolerance = defaultValue;
	
	sub.Normalize(tolerance);
	FRotator	newRotation(sub.Rotation());

	return newRotation;
}

//	change location * scale
template	<class T>
FVector		ACityGenerator::_getCoordLocation(int const i, T const obj)
{
	FVector 	location = FVector(obj.coordinates[i].x, obj.coordinates[i].y, 0.f);

	return location;
}

void	ACityGenerator::_generateDistrict(FDistrict	*district)
{
	_drawDistrictsBoundaries(district->neighbor.geom);
	// generate floor ?

	_generateBuildings(district->buildings);
	// // _generateRoads(district->roads);
	_generateTrees(district->trees);
	_generateBollards(district->bollards);
	_generateLights(district->lights);
}

//	en fonction du masque binaire -> renvoyer les data necessaires
//	manquantes
//	TODO: map
FString	ACityGenerator::_missingData(FDistrict const *district) const
{
	if (!district)
		return ("myDistricts");
	else if (district->buildings.IsEmpty())
		return ("buildings");
	else if (district->bollards.IsEmpty())
		return ("bollards");
	else if (district->lights.IsEmpty())
		return ("lights");
	return ("roads");
}

void ACityGenerator::_generateFromDT(UDataTable* districtTable)
{
	if (!districtTable)
	{
		UE_LOG(LogTemp, Error, TEXT("No data table found"));
		return;
	}
	
	TArray<FName>	rowNames = districtTable->GetRowNames();

	for (int i = 0; i < rowNames.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("DISTRICT name: %s"), *(rowNames[i]).ToString());
		FDistrict	*district = districtTable->FindRow<FDistrict>((rowNames[i]), "My district", true);
		if (!_checkAvailableData(district))
		{
			FString errorMsg = _missingData(district);
			UE_LOG(LogTemp, Error, TEXT("No %s data"), *errorMsg);
			return;
		}
		_generateDistrict(district);
	}
}


void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	_generateFromDT(myDistricts);
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// FVector MyCharacter = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	// UE_LOG(LogTemp, Display, TEXT("pos: %s"), *MyCharacter.ToString());

}

