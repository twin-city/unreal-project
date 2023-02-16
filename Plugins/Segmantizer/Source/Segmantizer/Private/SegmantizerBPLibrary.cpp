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

void USegmantizerBPLibrary::Save()
{
	FModuleManager::Get().GetModuleChecked<FSegmantizerModule>("Segmantizer").Save();
}

void USegmantizerBPLibrary::Capture()
{
	const FDateTime Now = FDateTime::Now();

	// store screenshot in Project directory next to main UProject/EXE based on the build type
	#if WITH_EDITOR
	const FString ImageDirectory = FString::Printf(TEXT("%s/%s"), *FPaths::ProjectDir(), TEXT("Screenshots"));
	#else
	const FString ImageDirectory = FString::Printf(TEXT("%s/../%s"), *FPaths::ProjectDir(), TEXT("Screenshots"));
	#endif

	const FString ImageFilename = FString::Printf(TEXT("%s/Screenshot_%d%02d%02d_%02d%02d%02d_%03d.png"), *ImageDirectory, Now.GetYear(), Now.GetMonth(), Now.GetDay(), Now.GetHour(), Now.GetMinute(), Now.GetSecond(), Now.GetMillisecond());

	FScreenshotRequest::RequestScreenshot(ImageFilename, false, false);
}