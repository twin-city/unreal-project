// Fill out your copyright notice in the Description page of Project Settings.

#include "MyBlueprintFunctionLibrary.h"
#include "Serialization/JsonSerializer.h"
#include "JsonObjectConverter.h"

void	UMyBlueprintFunctionLibrary::WriteStructToJsonFile(FString const FilePath, FMetaDatasStruct Struct)
{
    TSharedPtr<FJsonObject> JsonObject = FJsonObjectConverter::UStructToJsonObject(Struct);

    if (JsonObject == nullptr)
    {
        return;
    }
    
    WriteMetaDatasToFile(FilePath, JsonObject);
}

void	UMyBlueprintFunctionLibrary::WriteMetaDatasToFile(FString const FilePath, TSharedPtr<FJsonObject> JsonObject)
{
    FString JsonStr;

    if (!FJsonSerializer::Serialize(JsonObject.ToSharedRef(), TJsonWriterFactory<>::Create(&JsonStr, 0)))
    {
        return;
    }

    if (!FFileHelper::SaveStringToFile(JsonStr, *FilePath))
    {
        return;
    }
}