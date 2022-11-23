// Fill out your copyright notice in the Description page of Project Settings.


#include "../Common/SceneDistrict.h"

// Sets default values
ASceneDistrict::ASceneDistrict()
{
	PrimaryActorTick.bCanEverTick = false;

	ChildComponent = CreateDefaultSubobject<UChildActorComponent>(TEXT("DefaultRoot"));
	SetRootComponent(ChildComponent);
}


