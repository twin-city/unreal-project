#include "CityGenerator.h"
#include "ZoneShapeActor.h"
#include "ZoneShapeComponent.h"

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
		AZoneShape *zoneShape = GetWorld()->SpawnActor<AZoneShape>();
	
		UZoneShapeComponent *shape = Cast<UZoneShapeComponent>(zoneShape->GetComponentByClass(UZoneShapeComponent::StaticClass()));
	
		TArray<FZoneShapePoint>& points = shape->GetMutablePoints();
		
		const FRoad& road = roads[i];
		for (int j = 0; j < roads[i].coordinates.Num(); j++)
		{
			const FCoordinates& coords = road.coordinates[j];
			
			location = FVector(coords.x, coords.y, 0.5f);
			points.Push(FZoneShapePoint(location));
			
			if (j != 0)
				_spawnRoad(location, tmpLocation, roads[i].width);
			
			tmpLocation = location;
		}

		shape->UpdateShape();
	}
}

/************************************************/
/*                 COMMON						*/
/************************************************/

/*			CHECK VALUE			*/

bool	ACityGenerator::_isInNeighborhood(FDistrict* district) const
{
	return neighborsId.Find(district->id) != INDEX_NONE;
}

/*
	en fonction du masque binaire -> renvoyer les data necessaires manquantes
	TODO: map
*/
FString	ACityGenerator::_missingData(FDistrict* district) const
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
bool	ACityGenerator::_checkAvailableData(FDistrict* district) const
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

void	ACityGenerator::_generateDistrict(FDistrict* district)
{
	_generateBuildings(district->buildings);
	_generateRoads(district->roads);
	_generateObjects(district->lights, assets->lightActor);
	_generateObjects(district->bollards, assets->bollardActor);
	_generateObjects(district->trees, assets->treeActor);
	// _generateObjects(district->bus_shelters, assets->busShelterActor);
	_generateObjects(district->underground_stations, assets->undergroundStationActor);
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

void	ACityGenerator::_setDistrictActor(FDistrict *newDistrict)
{
	if (!newDistrict)
		return;
	
	FName			districtName = FName(newDistrict->name);

	UE_LOG(LogTemp, Display, TEXT("District name: %s"), *districtName.ToString());
	
	districtActor = GetWorld()->SpawnActor<ASceneDistrict>();
	districtActor->Rename(*newDistrict->name, REN_None);
	districtActor->SetActorLabel(newDistrict->name);
}

FDistrict	*ACityGenerator::_getChosenDistrict(FMyDataTable *city) const
{
	for (int i = 0; i < city->districts.Num(); i++)
	{
		if (FDistrict* chosenDistrict = city->districts[i]->FindRow<FDistrict>((choice), "My district", true))
			return chosenDistrict;
	}

	return nullptr;
}

void ACityGenerator::_generateNeighborhoodFromDT(FDistrict const &chosenDistrict)
{
	const FMyDataTable* cityData = cityTable->FindRow<FMyDataTable>((cityTable->GetRowNames())[0], "My city", true);

	_generateNeighborhoodFromCityDT(*cityData, chosenDistrict);
}

void ACityGenerator::_generateNeighborhoodFromCityDT(FMyDataTable const &inputCityTable, FDistrict const &chosenDistrict)
{
	neighborsId = chosenDistrict.neighbors;
	
	assets = assetTable->FindRow<FAssetTable>((assetTable->GetRowNames())[0], "My assets", true);
	
	for (UDataTable* districtTable : inputCityTable.districts)
	{
		FDistrict* district = districtTable->FindRow<FDistrict>((districtTable->GetRowNames())[0], "My district", true);
		
		_setDistrictActor(district);

		if (!_checkAvailableData(district))
		{
			FString errorMsg = _missingData(district);
			return;
		}
		_drawDistrictsBoundaries(district->geom, assets->coordActor);
		if (district->name == chosenDistrict.name || _isInNeighborhood(district))
			_generateDistrict(district);
	}
}

/*			MAIN FUNCTION			*/

void ACityGenerator::_generateFromCityDT(UDataTable	*inputCityTable)
{
	if (!assetTable || !inputCityTable)
	{
		UE_LOG(LogTemp, Error, TEXT("No data table found"));
		return;
	}

	if (FMyDataTable *city = inputCityTable->FindRow<FMyDataTable>((inputCityTable->GetRowNames())[0], "My city", true))
	{
		if (FDistrict *chosenDistrict = _getChosenDistrict(city))
			_generateNeighborhoodFromCityDT(*city, *chosenDistrict);
	}
}

/*			ORIGIN			*/

void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	_generateFromCityDT(cityTable);
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ACityGenerator::ACityGenerator()
{
	PrimaryActorTick.bCanEverTick = true;
}
