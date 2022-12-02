#include "CityGenerator.h"

/*
		GAME ART -> procedural building of roads + buildings
*/

/************************************************/
/*                	BOLLARDS					*/
/************************************************/

void	ACityGenerator::_generateBollards(TArray<FBollard> const &bollards)
{
	for (int i = 0; i < bollards.Num(); i++)
	{
		_setNewActor(bollards[i], defaultValue, assets->bollardActor);
	}
}

/************************************************/
/*                 ROADS						*/
/************************************************/

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
			
			location = FVector(coords.x, coords.y, 0.f);
			
			if (j != 0)
				_spawnRoad(location, tmpLocation, roads[i].width);
			
			tmpLocation = location;
		}
	}
}

/************************************************/
/*               TREES							*/
/************************************************/

void	ACityGenerator::_generateTrees(TArray<FTree> const &trees)
{
	for (int i = 0; i < trees.Num(); i++)
	{
		_setNewActor(trees[i], defaultValue, assets->treeActor);
	}
}

/************************************************/
/*               LIGHTS							*/
/************************************************/

void	ACityGenerator::_generateLights(TArray<FMyLight> const &lights)
{
	for (int i = 0; i < lights.Num(); i++)
	{
		_setNewActor(lights[i], lights[i].height, assets->lightActor);
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

bool	ACityGenerator::_isInDistrict(FVector pos)
{
	return true;
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

bool	ACityGenerator::_checkAvailableData() const
{
	//	TODO: masque binaire pour checker en fonction du quartier
	//	si les data necessaires existent bien
	if (!district)
		return false;
	//else if (district->buildings.IsEmpty())
	//	return false;
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
	FVector 	location = FVector(obj.coordinates[i].x, obj.coordinates[i].y, 0.f); // - offset;

	return location;
}

void	ACityGenerator::_generateDistrict()
{
	// newPos = GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation();
	// district->isConstructed = false;

	// for (int i = 0; i < district->geom.coordinates.Num(); i++)
	// {
	// 	for (int j = 0; j < district->geom.coordinates.Num(); j++)
	// 	{
	// 		// if (!district->isConstructed && ((newPos.X - district->geom.coordinates[i].x * scale < 1000 && newPos.X - district->geom.coordinates[i].x * scale > -1000) || (district->geom.coordinates[i].x * scale - newPos.X < 1000 && district->geom.coordinates[i].x * scale - newPos.X > -1000)) 
	// 		// 	&& ((newPos.Y - district->geom.coordinates[j].y * scale < 1000 && newPos.Y - district->geom.coordinates[j].y * scale > -1000) || (district->geom.coordinates[j].y * scale - newPos.Y < 1000 && district->geom.coordinates[j].y * scale - newPos.Y > -1000)))
	// 		{
	// 			_generateBuildings(district->buildings, districtActor);
	// 			// // _generateRoads(district->roads, districtActor);
	// 			// _generateTrees(district->trees, districtActor);
	// 			// _generateBollards(district->bollards, districtActor);
	// 			// _generateLights(district->lights, districtActor);
	// 			// district->isConstructed = true;
	// 			return ;
	// 		}
	// 	}
	// }
	// if (district->isConstructed)
	// {
	// 	//	remove district
	// 	district->isConstructed = false;
	// 	UE_LOG(LogTemp, Display, TEXT("District %s has been removed"), *district->name);
	// }
}

//	en fonction du masque binaire -> renvoyer les data necessaires
//	manquantes
//	TODO: map
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

FDistrict	*ACityGenerator::_newDistrictActor(UDataTable *newDistrict)
{
	FName			districtName = newDistrict->GetRowNames()[0];

	UE_LOG(LogTemp, Display, TEXT("dis name: %s"), *districtName.ToString());
		
	district = newDistrict->FindRow<FDistrict>((districtName), "My district", true);
	
	districtActor = GetWorld()->SpawnActor<ASceneDistrict>();
	districtActor->Rename(*district->name, REN_None);
	districtActor->SetActorLabel(district->name);

	return district;
}

void ACityGenerator::_generateFromDT(UDataTable	*districtTable)
{
	UE_LOG(LogTemp, Warning, TEXT("Your message"));
	if (!assetTable || !districtTable)
	{
		UE_LOG(LogTemp, Error, TEXT("No data table found"));
		return;
	}

	assets = assetTable->FindRow<FAssetTable>((assetTable->GetRowNames())[0], "My assets", true);
	FMyDataTable	*city = districtTable->FindRow<FMyDataTable>((districtTable->GetRowNames())[0], "My city", true);
	// FDistrict	*district = districtTable->FindRow<FDistrict>((rowNames[i]), "My district", true);
	for (int i = 0; i < city->districts.Num(); i++)
	{
		_newDistrictActor(city->districts[i]);

		if (!_checkAvailableData())
		{
			FString errorMsg = _missingData();
			return;
		}
		_drawDistrictsBoundaries(district->geom, assets->coordActor);
		_generateDistrict();
	}
}

void ACityGenerator::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("BEGIN"));
	
	_generateFromDT(cityTable);
}

void ACityGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
