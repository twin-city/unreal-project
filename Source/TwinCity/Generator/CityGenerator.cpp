#include "CityGenerator.h"

/*
		GAME ART -> procedural building of roads + buildings
*/

/************************************************/
/*               POPULATE						*/
/************************************************/

/*			BUILDINGS			*/

void		ACityGenerator::_spawnWall(FVector const &v1, FVector const &v2, float const height)
{
	FVector		meanVector = FMath::Lerp(v1, v2, 0.5f) * scale;
	/*	TODO: a rechecker	*/
	meanVector.Z = height * scale / 2;
	FRotator	newRotation = _getNewRotation(v1, v2);
	float		width = FVector::Dist(v1, v2);
	AActor		*myActor = GetWorld()->SpawnActor<AActor>(assets->wallActor, meanVector, newRotation);
	FVector		newScale = FVector(width, 1.f, height);	// scale -> defaultDepth

	myActor->SetActorScale3D(newScale);
	myActor->AttachToActor(districtActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
}

void	ACityGenerator::_generateBuildings(TArray<FBuilding> const &buildings)
{
	FVector		location, tmpLocation;

	//	for each buildings
	for (int i = 0; i < buildings.Num(); i++)
	{
		//	for each vertices
		for (int j = 0; j < buildings[i].coordinates.Num(); j++)
		{
			location = _getCoordLocation(j, buildings[i]);
			if (j > 0)
			{
				_spawnWall(location, tmpLocation, buildings[i].height);	// to change in prepare_data
			}
			tmpLocation = location;
		}
	}
}

/*			ROADS			*/

void	ACityGenerator::_spawnRoad(FVector const &v1, FVector const &v2, float const depth)
{
	FVector		meanVector = FMath::Lerp(v1, v2, 0.5f) * scale;
	
	FRotator	newRotation = _getNewRotation(v1, v2);
	float		width = FVector::Dist(v1, v2);
	AActor		*myActor = GetWorld()->SpawnActor<AActor>(assets->roadActor, meanVector, newRotation);
	FVector		newScale = FVector(width, depth, defaultValue);

	myActor->SetActorScale3D(newScale);
	myActor->AttachToActor(districtActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
}

void	ACityGenerator::_generateRoads(TArray<FRoad> const &roads)
{
	FVector	location, tmpLocation;

	for (int i = 0; i < roads.Num(); i++)
	{
		const FRoad& road = roads[i];
		for (int j = 0; j < roads[i].coordinates.Num(); j++)
		{
			const FCoordinates& coords = road.coordinates[j];
			
			location = FVector(coords.x, coords.y, 0.5f);
			
			if (j != 0)
				_spawnRoad(location, tmpLocation, roads[i].width);
			
			tmpLocation = location;
		}
	}
}

/************************************************/
/*                 COMMON						*/
/************************************************/

/*			CHECK VALUE			*/

bool	ACityGenerator::_isInNeighborhood() const
{
	if (neighborsId.Find(district->id) != INDEX_NONE)
	{
		return true;
	}

	return false;
}

/*
	en fonction du masque binaire -> renvoyer les data necessaires manquantes
	TODO: map
*/
FString	ACityGenerator::_missingData() const
{
	if (!district)
		return ("myDistricts");
	else if (district->buildings.IsEmpty())
		return ("buildings");
	else if (district->roads.IsEmpty())
		return ("roads");
	else if (district->bollards.IsEmpty())
		return ("bollards");
	else if (district->lights.IsEmpty())
		return ("lights");
	return ("roads");
}

/*
	TODO: masque binaire pour checker en fonction du quartier
	si les data necessaires existent bien
*/
bool	ACityGenerator::_checkAvailableData() const
{
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

/*			GET VALUE			*/

FRotator	ACityGenerator::_getNewRotation(FVector const &v1, FVector const &v2)
{
	FVector sub = v1 - v2;
	float	tolerance = defaultValue;
	
	sub.Normalize(tolerance);
	FRotator	newRotation(sub.Rotation());

	return newRotation;
}

/*
	useless ?
*/
template	<class T>
FVector		ACityGenerator::_getCoordLocation(int const i, T const obj)
{
	FVector 	location = FVector(obj.coordinates[i].x, obj.coordinates[i].y, 0.f);

	return location;
}

/*			SET VALUE			*/

void	ACityGenerator::_generateDistrict()
{
	_generateBuildings(district->buildings);
	_generateRoads(district->roads);
	_generateObjects(district->lights, assets->lightActor);
	_generateObjects(district->bollards, assets->bollardActor);
	_generateObjects(district->trees, assets->treeActor);
}

void	ACityGenerator::_drawDistrictsBoundaries(FGeom const &geom, TSubclassOf<AActor> const &actorToSpawn)
{
	FVector location = FVector::ZeroVector;

	for (int i = 0; i < geom.coordinates.Num(); i++)
	{
		location = _getCoordLocation(i, geom) * scale;
		
		AActor* bound = GetWorld()->SpawnActor<AActor>(actorToSpawn, location, FRotator::ZeroRotator);
		bound->AttachToActor(districtActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
	}
}

template	<class T>
void	ACityGenerator::_setNewActor(T const obj, float height, TSubclassOf<AActor> const &actorToSpawn)
{
	for (int i = 0; i < obj.coordinates.Num(); i++)
	{
		FVector	location = _getCoordLocation(i, obj) * scale;
		AActor	*myActor = GetWorld()->SpawnActor<AActor>(actorToSpawn, location, FRotator::ZeroRotator);
		FVector	newScale = FVector(defaultValue, defaultValue, height);

		myActor->AttachToActor(districtActor, FAttachmentTransformRules(EAttachmentRule::KeepWorld, false));
		
		if (height != defaultValue)
			myActor->SetActorScale3D(newScale);
	}
}

template	<class T>
void	ACityGenerator::_generateObjects(TArray<T> const &obj, TSubclassOf<AActor> const &actorToSpawn)
{
	for (int i = 0; i < obj.Num(); i++)
	{
		_setNewActor(obj[i], defaultValue, actorToSpawn);
	}

}

void	ACityGenerator::_setDistrictActor(UDataTable *newDistrict)
{
	FName			districtName = newDistrict->GetRowNames()[0];

	UE_LOG(LogTemp, Display, TEXT("District name: %s"), *districtName.ToString());

	district = newDistrict->FindRow<FDistrict>((districtName), "My district", true);
	districtActor = GetWorld()->SpawnActor<ASceneDistrict>();
	districtActor->Rename(*district->name, REN_None);
	districtActor->SetActorLabel(district->name);
}

FDistrict	*ACityGenerator::_setChosenDistrict(FMyDataTable *city)
{
	FDistrict *chosenDistrict = nullptr;

	for (int i = 0; i < city->districts.Num(); i++)
	{
		if ((chosenDistrict = city->districts[i]->FindRow<FDistrict>((choice), "My district", true)))	
		{
			neighborsId = chosenDistrict->neighbors;
			break ;	
		}
	}

	return chosenDistrict;
}

/*			MAIN FUNCTION			*/

void ACityGenerator::_generateFromDT(UDataTable	*districtTable)
{
	if (!assetTable || !districtTable)
	{
		UE_LOG(LogTemp, Error, TEXT("No data table found"));
		return;
	}

	FMyDataTable	*city = districtTable->FindRow<FMyDataTable>((districtTable->GetRowNames())[0], "My city", true);
	FDistrict		*chosenDistrict = _setChosenDistrict(city);
	
	assets = assetTable->FindRow<FAssetTable>((assetTable->GetRowNames())[0], "My assets", true);
	
	if (!chosenDistrict)
	{
		UE_LOG(LogTemp, Error, TEXT("%s district does not exist"), *choice.ToString());
		return;
	}

	for (int i = 0; i < city->districts.Num(); i++)
	{
		_setDistrictActor(city->districts[i]);

		if (!_checkAvailableData())
		{
			FString errorMsg = _missingData();
			return;
		}
		_drawDistrictsBoundaries(district->geom, assets->coordActor);
		if ((chosenDistrict && district->name == chosenDistrict->name) || _isInNeighborhood())
			_generateDistrict();
	}
}

/*			ORIGIN			*/

void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	_generateFromDT(cityTable);
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ACityGenerator::ACityGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
}
