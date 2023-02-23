// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

#include "Math/IntVector.h"
#include "RHIDefinitions.h"

class FRHIShaderResourceView;
class FRHIUnorderedAccessView;

class CITYSAMPLESENSORGRIDSHADERS_API FCitySampleSensorGridHelper
{
public:
	FCitySampleSensorGridHelper(ERHIFeatureLevel::Type InFeatureLevel, const FUintVector4& InSensorGridDimensions, uint32 FrameIndex);
	~FCitySampleSensorGridHelper();

	struct FResourceSizingInfo
	{
		uint32 SensorCount = 0;
		uint32 OwnerCount = 0;
	};

	struct FTransientResources
	{
		FTransientResources() = default;

		FTransientResources(const FTransientResources&) = delete;
		FTransientResources& operator=(const FTransientResources&) = delete;

		FTransientResources(FTransientResources&& InResources) = default;
		FTransientResources& operator=(FTransientResources&&) = default;

		CITYSAMPLESENSORGRIDSHADERS_API bool Supports(const FResourceSizingInfo& SizingInfo) const;
		CITYSAMPLESENSORGRIDSHADERS_API void Reset();
		CITYSAMPLESENSORGRIDSHADERS_API void Build(const FResourceSizingInfo& SizingInfo);
		CITYSAMPLESENSORGRIDSHADERS_API void ResetTransitions(FRHICommandList& RHICmdList);

		TUniquePtr<FRWBuffer> PartialBounds;
		TUniquePtr<FRWBuffer> LeafIndices[2];
		TUniquePtr<FRWBuffer> MortonCodes[2];
		TUniquePtr<FRWBuffer> DuplicateCounts;
		TUniquePtr<FRWBuffer> CopyCommands;
		TUniquePtr<FRWBuffer> ParentIndices;
		TUniquePtr<FRWBuffer> HierarchyGates;
		TUniquePtr<FRWBuffer> OwnerBoundingBoxes;
		TUniquePtr<FRWBufferStructured> InternalNodes;
		TUniquePtr<FRWBuffer> SensorCounts;

		FResourceSizingInfo SizingInfo;
		bool HasBuffers = false;
	};

	static uint32 GetMaxSensorDensity();
	static uint32 GetMaxOwnerCount();

	void ResetLeafBounds(
		FRHICommandList& RHICmdList,
		FRHIUnorderedAccessView* SensorLocationsUav);

	void ResetResults(
		FRHICommandList& RHICmdList,
		FRHIUnorderedAccessView* ResultsUav);

	void NearestSensors(
		FRHICommandList& RHICmdList,
		const FVector2D& GlobalSensorRange,
		FTransientResources& TransientResources,
		FRHIShaderResourceView* SensorLocationsSrv,
		FRHIUnorderedAccessView* ResultsUav);

	void GenerateOwnerBounds(
		FRHICommandList& RHICmdList,
		FTransientResources& TransientResources,
		FRHIShaderResourceView* SensorLocationsSrv);

	void GenerateSortedLeaves(
		FRHICommandList& RHICmdList,
		FTransientResources& TransientResources,
		FRHIShaderResourceView* SensorLocationsSrv);

	void GenerateBvh(
		FRHICommandList& RHICmdList,
		FTransientResources& TransientResources,
		FRHIShaderResourceView* SensorLocationsSrv);

	void RunTraversals(
		FRHICommandList& RHICmdList,
		const FVector2D& GlobalSensorRange,
		FTransientResources& TransientResources,
		FRHIShaderResourceView* SensorLocationsSrv,
		FRHIUnorderedAccessView* ResultsUav);

private:
	const ERHIFeatureLevel::Type FeatureLevel;
	const FUintVector4 SensorGridDimensions;
};
