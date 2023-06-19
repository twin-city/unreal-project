// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

/**
 * 
 */

class FJsonObject;

USTRUCT(BlueprintType, Category="MetaDatasStruct")
struct FMetaDatasStruct
{
	GENERATED_USTRUCT_BODY()

	public:

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		FString	District = "";

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		int	Hour = 0;
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		int	Day = 0;
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		int	Month = 0;
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		int	Year = 0;

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		FString	Weather = "";

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		FRotator	CameraRotation = { 0.f, 0.f, 0.f };

		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		FVector	CameraLocation = { 0.f, 0.f, 0.f };
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		int	VehiclesNb = 0;
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		TMap<int, FString> Peds;
		
		UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Data")
		TMap<int, FString> LyingPeds;

};

UCLASS()
class TWINCITY_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	public:

		UFUNCTION(BlueprintCallable, Category="MetaDatas")
		static void	WriteStructToJsonFile(FString const FilePath, FMetaDatasStruct Struct);
	
		static void	WriteMetaDatasToFile(FString const FilePath, TSharedPtr<FJsonObject> JsonObject);
	
};
