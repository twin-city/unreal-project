// Fill out your copyright notice in the Description page of Project Settings.


#include "Clock.h"

// Sets default values
AClock::AClock()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AClock::BeginPlay()
{
	Super::BeginPlay();
	
	BeginTime = FPlatformTime::Seconds();
	
}

// Called every frame
void AClock::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!IsTimePassing && TmpHours == -1.f)
	{
		TmpTime = CurrentTime;
		HasChanged = true;
		return ;
	}

	if (HasChanged)
	{
		BeginTime = FPlatformTime::Seconds() - CurrentHours;
		HasChanged = false;
	}

	CurrentTime = FTimespan(FTimespan::FromHours(FPlatformTime::Seconds() - BeginTime));
	CurrentDays = CurrentTime.GetDays();
	CurrentMinutes = CurrentTime.GetMinutes();
	
	if (TmpHours == -1.f)
		CurrentHours = CurrentTime.GetHours();
	else
	{
		CurrentHours = TmpHours;
		TmpHours = -1.f;
		BeginTime = FPlatformTime::Seconds() - CurrentHours;
	}
	CurrentTime = FTimespan(CurrentDays, CurrentHours, CurrentMinutes, 0);
}

// Called to bind functionality to input
void AClock::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

