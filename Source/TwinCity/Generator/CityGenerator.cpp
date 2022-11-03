#include "CityGenerator.h"

/*
	TODO:
		- GET x1, y1 & x2, y2 + create grid pattern of the neighbourhood
		- GET my player location
		- LOAD neighbourhood of my district
		- GENERATE() my district when the player is closed
*/

/************************************************/
/*                	BOLLARDS					*/
/************************************************/

void	ACityGenerator::_generateBollards(TArray<FBollard> bollards)
{
	for (int i = 0; i < bollards.Num(); i++)
	{
		_setNewActor(bollards[i].coordonnees, defaultValue, bollardActor);
	}
}

/************************************************/
/*                 ROADS						*/
/************************************************/

void	ACityGenerator::_spawnRoad(FVector const &v1, FVector const &v2, float const depth)
{
	FVector		meanVector = FMath::Lerp(v1, v2, 0.5f) * scale;
	meanVector.Z = defaultHeight;
	FRotator	newRotation = _getNewRotation(v1, v2);
	float		width = FVector::Dist(v1, v2);
	//	checker si possible de spawn et rescale en meme temps
	AActor		*myActor = GetWorld()->SpawnActor<AActor>(roadActor, meanVector, newRotation);
	FVector		newScale = FVector(width, depth, defaultValue);

	myActor->SetActorScale3D(newScale);
}

void	ACityGenerator::_generateRoads(TArray<FRoad> roads)
{
	FVector	location, tmpLocation;

	for (int i = 0; i < roads.Num(); i++)
	{
		for (int j = 0; j < roads[i].coordonnees.Num(); j++)
		{
			location = _getCoordLocation(j, roads[i]);
			// _spawnCoord(Location);
			if (j > 0)
				_spawnRoad(location, tmpLocation, roads[i].largeur_de_chaussee);
				
			tmpLocation = location;
		}
	}
}

/************************************************/
/*               TREES							*/
/************************************************/

void	ACityGenerator::_generateTrees(TArray<FTree> trees)
{
	for (int i = 0; i < trees.Num(); i++)
	{
		_setNewActor(trees[i].coordonnees, defaultValue, treeActor);
		// _setNewActor(trees->data[i].coordonnees, trees->data[i].hauteurenm, treeActor);
	}
}

/************************************************/
/*               LIGHTS							*/
/************************************************/

void	ACityGenerator::_generateLights(TArray<FMyLight> lights)
{
	for (int i = 0; i < lights.Num(); i++)
	{
		_setNewActor(lights[i].coordonnees, lights[i].hauteur, lightActor);
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
		for (int j = 0; j < buildings[i].coordonnees.Num(); j++)
		{
			location = _getCoordLocation(j, buildings[i]);
			_spawnCoord(location * scale + buildings[i].hauteur * FVector::UpVector * scale); // to change in prepare_data
			if (j > 0)
			{
				_spawnWall(location, tmpLocation, buildings[i].hauteur);	// to change in prepare_data
			}
			tmpLocation = location;
		}
	}
}

/************************************************/
/*                 DEBUG						*/
/************************************************/

void	ACityGenerator::_spawnCoord(FVector const &location)
{
	GetWorld()->SpawnActor<AActor>(coordActor, location, FRotator::ZeroRotator);
}

/************************************************/
/*                 COMMON						*/
/************************************************/

// GET x1, y1 & x2, y2 + create grid pattern of the neighbourhood

// FMatrix2x2	ACityGenerator::_getBoundariesCoord(FDistrict const district)
// {
// 	FMatrix2x2	matrix;

// }

void	ACityGenerator::_drawDistrictsBoundaries() const
{
	TArray<FName>	rowNames = myDistrict->GetRowNames();

	for (int i = 0; i < rowNames.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("name: %s"), *(rowNames[i]).ToString());
		//	_getDistrictCoord();
		//	_spawnCoord();
		//	_spawnWall(); -> with height == 1
	}
}

void	ACityGenerator::_setNewActor(FCoordonnees coord, float height, TSubclassOf<AActor> actorToSpawn)
{
	FVector 	location = (FVector(coord.x, coord.y, 0.f) + offset) * scale;	
	FRotator	rotation = FRotator::ZeroRotator;
	FVector		newScale = FVector(defaultValue, defaultValue, height);
	AActor		*myActor = GetWorld()->SpawnActor<AActor>(actorToSpawn, location, rotation);

	if (height == defaultValue)
		return;
	myActor->SetActorScale3D(newScale);
}

bool	ACityGenerator::_checkAvailableData(FDistrict const *district) const
{
	//	TODO: masque binaire pour checker en fonction du quartier
	//	si les data necessaires existent bien
	if (!myDistrict)
		return false;
	else if (!district)
		return false;
	else if (district->buildings.IsEmpty())
		return false;
	else if (district->roads.IsEmpty())
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
	FVector 	location = (FVector(obj.coordonnees[i].x, obj.coordonnees[i].y, 0.f) + offset); // to change in prepare_data

	return location;
}

void	ACityGenerator::_generate(FDistrict	*district)
{
	_drawDistrictsBoundaries();
	// generate floor ?
	_generateBuildings(district->buildings);
	_generateRoads(district->roads);
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
		return ("myDistrict");
	else if (district->buildings.IsEmpty())
		return ("buildings");
	else if (district->bollards.IsEmpty())
		return ("bollards");
	else if (district->lights.IsEmpty())
		return ("lights");
	return ("roads");
}

void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();

	// LOAD only one district

	// if (myChoice == "")
	// {
	// 	UE_LOG(LogTemp, Error, TEXT("No district"));
	// 	return ;
	// }
	
	// FDistrict	*district = myDistrict->FindRow<FDistrict>(myChoice, "My district", true);

	// if (!_checkAvailableData(district))
	// {
	// 	FString errorMsg = _missingData(district);
	// 	UE_LOG(LogTemp, Error, TEXT("No %s data"), *errorMsg);
	// 	return;
	// }
	// _generate(district);

	// LOAD every districts
	TArray<FName>	rowNames = myDistrict->GetRowNames();

	for (int i = 0; i < rowNames.Num(); i++)
	{
		UE_LOG(LogTemp, Display, TEXT("DISTRICT name: %s"), *(rowNames[i]).ToString());
		FDistrict	*district = myDistrict->FindRow<FDistrict>((rowNames[i]), "My district", true);

		if (!_checkAvailableData(district))
		{
			FString errorMsg = _missingData(district);
			UE_LOG(LogTemp, Error, TEXT("No %s data"), *errorMsg);
			return;
		}
		_generate(district);
	}
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

