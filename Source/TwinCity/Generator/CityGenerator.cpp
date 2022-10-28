#include "CityGenerator.h"

/************************************************/
/*                	BOLLARDS					*/
/************************************************/

void	ACityGenerator::_generateBollards()
{
	FGlobalBollards	*bol =  myBollards->FindRow<FGlobalBollards>(FName(TEXT("saintaugustin")), "My bollards", true);

	for (int i = 0; i < bol->data.Num(); i++)
	{
		_setNewActor(bol->data[i].coordonnees, defaultValue, bollardActor);
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

void	ACityGenerator::_generateRoads()
{
	FGlobalRoads	*roads =  myRoads->FindRow<FGlobalRoads>(FName(TEXT("saintaugustin")), "My roads", true);
	FVector			location, tmpLocation;

	for (int i = 0; i < roads->data.Num(); i++)
	{
		for (int j = 0; j < roads->data[i].coordonnees.Num(); j++)
		{
			location = _getCoordLocation(i, j, roads);
			// _spawnCoord(Location);
			if (j > 0)
				_spawnRoad(location, tmpLocation, roads->data[i].largeur_de_chaussee);
				
			tmpLocation = location;
		}
	}
}

/************************************************/
/*               TREES							*/
/************************************************/

void	ACityGenerator::_generateTrees()
{
	FGlobalTree	*trees =  myTrees->FindRow<FGlobalTree>(FName(TEXT("saintaugustin")), "My trees", true);

	for (int i = 0; i < trees->data.Num(); i++)
	{
		_setNewActor(trees->data[i].coordonnees, defaultValue, treeActor);
		// _setNewActor(trees->data[i].coordonnees, trees->data[i].hauteurenm, treeActor);
	}
}

/************************************************/
/*               LIGHTS							*/
/************************************************/

void	ACityGenerator::_generateLights()
{
	FGlobalLights	*lights =  myLights->FindRow<FGlobalLights>(FName(TEXT("saintaugustin")), "My lights", true);

	for (int i = 0; i < lights->data.Num(); i++)
	{
		_setNewActor(lights->data[i].coordonnees, lights->data[i].hauteur, lightActor);
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

void	ACityGenerator::_generateBuildings()
{
	FGlobalBuildings	*buildings =  myBuildings->FindRow<FGlobalBuildings>(FName(TEXT("saintaugustin")), "My buildings", true);
	FVector				location, tmpLocation;

	//	for each buildings
	for (int i = 0; i < buildings->data.Num(); i++)
	{
		//	for each vertices
		for (int j = 0; j < buildings->data[i].coordonnees.Num(); j++)
		{
			location = _getCoordLocation(i, j, buildings);
			_spawnCoord(location * scale + buildings->data[i].hauteur * FVector::UpVector * scale); // to change in prepare_data
			if (j > 0)
			{
				_spawnWall(location, tmpLocation, buildings->data[i].hauteur);	// to change in prepare_data
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

bool	ACityGenerator::_checkAvailableData() const
{
	//	TODO: masque binaire pour checker en fonction du quartier
	//	si les data necessaires existent bien

	return myBuildings && myBollards && myRoads && myLights;
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
FVector		ACityGenerator::_getCoordLocation(int const i, int const j, T const *obj)
{
	FVector 	location = (FVector(obj->data[i].coordonnees[j].x, obj->data[i].coordonnees[j].y, 0.f) + offset); // to change in prepare_data

	return location;
}

void	ACityGenerator::_generate()
{
	_generateBuildings();
	_generateRoads();
	_generateTrees();
	_generateBollards();
	_generateLights();
}

//	en fonction du masque binaire -> renvoyer les data necessaires
//	manquantes
//	TODO: map
FString	ACityGenerator::_missingData()
{
	if (!myBuildings)
		return ("buildings");
	else if (!myBollards)
		return ("bollards");
	else if (!myLights)
		return ("lights");
	return ("roads");
}

void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();

	if (!_checkAvailableData())
	{
		FString errorMsg = _missingData();
		UE_LOG(LogTemp, Error, TEXT("No %s data"), *errorMsg);
		return;
	}
	_generate();
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

