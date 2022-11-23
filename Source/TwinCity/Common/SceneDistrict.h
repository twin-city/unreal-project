// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneDistrict.generated.h"

UCLASS()
class TWINCITY_API ASceneDistrict : public AActor
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	UChildActorComponent * ChildComponent = nullptr;
	
	// Sets default values for this actor's properties
	ASceneDistrict();

};
