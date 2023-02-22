// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Actor.h"
#include "Engine/DirectionalLight.h"
#include "Misc/OutputDeviceNull.h"
#include "Misc/Timespan.h" 
#include "Clock.generated.h"

UCLASS()
class TWINCITY_API AClock : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AClock();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	double				BeginTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock")
	double				CurrentMinutes;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock")
	double				CurrentHours;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock")
	double				TmpHours = -1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock")
	double				CurrentDays;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Clock")
	FTimespan			CurrentTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky")
  	ADirectionalLight	*LightSource;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky")
  	AActor				*Sun;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sky")
  	float				TurnRate;

};
