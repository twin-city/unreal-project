// Copyright Epic Games, Inc. All Rights Reserved.

#include "TwinCityGameMode.h"
#include "TwinCityCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATwinCityGameMode::ATwinCityGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
