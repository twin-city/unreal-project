// Copyright Epic Games, Inc. All Rights Reserved.

#include "SegmantizerBPLibrary.h"
#include "Segmantizer.h"

USegmantizerBPLibrary::USegmantizerBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

void USegmantizerBPLibrary::SetViewToSemantic()
{
	FModuleManager::Get().GetModuleChecked<FSegmantizerModule>("Segmantizer").SetViewToSemantic();
}

void USegmantizerBPLibrary::SetViewToLit()
{
	FModuleManager::Get().GetModuleChecked<FSegmantizerModule>("Segmantizer").SetViewToLit();
}

void USegmantizerBPLibrary::SaveClassDataAsset()
{
	FModuleManager::Get().GetModuleChecked<FSegmantizerModule>("Segmantizer").SaveClassDataAsset();
}

void USegmantizerBPLibrary::Capture()
{
	FModuleManager::Get().GetModuleChecked<FSegmantizerModule>("Segmantizer").CaptureStart();
}