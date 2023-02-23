// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleSensorGridShaders.h"

#include "Algo/RemoveIf.h"
#include "GPUSort.h"
#include "ShaderPermutation.h"
#include "ShaderParameterStruct.h"

namespace CitySampleSensorGridShaders
{
	static const uint32 SensorsPerOwnerAlignment = 128;
	static const uint32 MortonCompactionBufferSize = 128;
	static const uint32 MortonCodeBitsReservedForOwner = 5;

	// todo - look at collapsing to 16 bytes (2 x half3 + 2 x int16)
	struct alignas(16) FInternalNode
	{
		FVector BoundsMin;
		int32 LeftChild;

		FVector BoundsMax;
		int32 RightChild;
	};
};

static bool GCitySampleSensorGridBuildDisabled = false;
static FAutoConsoleVariableRef CVarCitySampleSensorGridBuildDisabled(
	TEXT("CitySample.sensorgrid.BuildDisabled"),
	GCitySampleSensorGridBuildDisabled,
	TEXT("When true the sensor grid will not be build or traversed."),
	ECVF_Default
);

class FCitySampleSensorGridResetSensorLocationsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridResetSensorLocationsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridResetSensorLocationsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_UAV(Buffer<float4>, SensorsToReset)
		SHADER_PARAMETER(uint32, SensorCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("RESET_SENSOR_LOCATIONS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("RESET_SENSOR_LOCATIONS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridResetSensorLocationsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "ResetSensorLocations", SF_Compute);

class FCitySampleSensorGridClearResultsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridClearResultsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridClearResultsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_UAV(StructuredBuffer<FSensorInfo>, NearestSensors)
		SHADER_PARAMETER(uint32, SensorCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("CLEAR_RESULTS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("CLEAR_RESULTS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridClearResultsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "ClearNearestSensors", SF_Compute);

class FCitySampleSensorGridBvhPrimeBoundsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhPrimeBoundsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhPrimeBoundsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_UAV(Buffer<float4>, PartialBoundingBoxes)
		SHADER_PARAMETER(uint32, SensorCount)
		SHADER_PARAMETER(uint32, PaddedIntermediateCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("PRIME_BOUNDS_GENERATION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("PRIME_BOUNDS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhPrimeBoundsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "PrimeBounds", SF_Compute);

class FCitySampleSensorGridBvhFinalizeBoundsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhFinalizeBoundsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhFinalizeBoundsCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<float4>, SourceBoundingBoxes)
		SHADER_PARAMETER_UAV(Buffer<float4>, TargetBoundingBoxes)
		SHADER_PARAMETER(uint32, SourceBoundsCount)
		SHADER_PARAMETER(uint32, PaddedSourceBoundsCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("FINALIZE_BOUNDS_CS"), 1);
		OutEnvironment.SetDefine(TEXT("FINALIZE_BOUNDS_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhFinalizeBoundsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "FinalizeBounds", SF_Compute);


class FCitySampleSensorGridBvhMortonCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhMortonCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhMortonCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_SRV(Buffer<float4>, BoundingBoxes)
		SHADER_PARAMETER_UAV(Buffer<uint>, LeafIndices)
		SHADER_PARAMETER_UAV(Buffer<uint>, MortonCodes)
		SHADER_PARAMETER(uint32, SensorCount)
		SHADER_PARAMETER(uint32, PaddedOutputCount)
		SHADER_PARAMETER(uint32, OwnerBitCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_GENERATTION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhMortonCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonGeneration", SF_Compute);

class FCitySampleSensorGridMortonCompactionCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridMortonCompactionCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridMortonCompactionCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, InputValues)
		SHADER_PARAMETER_UAV(Buffer<uint>, OutputValues)
		SHADER_PARAMETER_UAV(Buffer<uint>, DuplicateCounts)
		SHADER_PARAMETER(uint32, ValueCount)
		SHADER_PARAMETER(uint32, OwnerBitCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_COMPACTION_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_COMPACTION_CHUNK_SIZE"), CitySampleSensorGridShaders::MortonCompactionBufferSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridMortonCompactionCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonCompaction", SF_Compute);

class FCitySampleSensorGridBuildCopyCommandsCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBuildCopyCommandsCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBuildCopyCommandsCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, DuplicateCounts)
		SHADER_PARAMETER_SRV(Buffer<uint>, CompactedValues)
		SHADER_PARAMETER_UAV(RWBuffer<uint4>, CopyCommands)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, ElementsPerOwner)
		SHADER_PARAMETER(uint32, OwnerCount)
		SHADER_PARAMETER(uint32, GroupsPerOwner)
		SHADER_PARAMETER(uint32, MaxElementsPerGroup)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_BUILD_COPY_COMMANDS_CS"), 1);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBuildCopyCommandsCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BuildCopyCommands", SF_Compute);

class FCitySampleSensorGridShuffleDataCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridShuffleDataCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridShuffleDataCs, FGlobalShader);

public:
	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, InputValues)
		SHADER_PARAMETER_SRV(Buffer<uint4>, CopyCommands)
		SHADER_PARAMETER_UAV(RWBuffer<uint>, OutputValues)
		SHADER_PARAMETER(uint32, ValueCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("MORTON_SHUFFLE_DATA_CS"), 1);
		OutEnvironment.SetDefine(TEXT("MORTON_SHUFFLE_CHUNK_SIZE"), CitySampleSensorGridShaders::MortonCompactionBufferSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridShuffleDataCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "MortonShuffleData", SF_Compute);

class FCitySampleSensorGridBvhGenTopDownCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhGenTopDownCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhGenTopDownCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, LeafCounts)
		SHADER_PARAMETER_SRV(Buffer<uint>, LeafIndices)
		SHADER_PARAMETER_SRV(Buffer<uint>, MortonCodes)
		SHADER_PARAMETER_UAV(StructuredBuffer<FInternalNode>, InternalNodes)
		SHADER_PARAMETER_UAV(Buffer<uint>, ParentIndices)
		SHADER_PARAMETER_UAV(Buffer<uint>, AccumulationGates)
		SHADER_PARAMETER(uint32, InternalNodeParentOffset)
		SHADER_PARAMETER(uint32, PaddedLeafNodeCount)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, PaddedParentCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("HIERARCHY_GENERATION_TOP_DOWN_CS"), 1);
		OutEnvironment.SetDefine(TEXT("HIERARCHY_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhGenTopDownCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "HierarchyGeneration_TopDown", SF_Compute);

class FCitySampleSensorGridBvhGenBottomUpCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhGenBottomUpCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhGenBottomUpCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 128;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, SensorCounts)
		SHADER_PARAMETER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_SRV(Buffer<uint>, ParentIndices)
		SHADER_PARAMETER_UAV(StructuredBuffer<FInternalNode>, InternalNodes)
		SHADER_PARAMETER_UAV(Buffer<UINT>, AccumulationGates)
		SHADER_PARAMETER(uint32, InternalNodeParentOffset)
		SHADER_PARAMETER(uint32, MaxSensorsPerOwner)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, PaddedParentCount)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("BOUNDS_GENERATION_BOTTOM_UP_CS"), 1);
		OutEnvironment.SetDefine(TEXT("BOUNDS_GENERATION_CHUNK_SIZE"), ChunkSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhGenBottomUpCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BoundsGeneration_BottomUp", SF_Compute);

class FCitySampleSensorGridBvhTraversalCs : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCitySampleSensorGridBvhTraversalCs);
	SHADER_USE_PARAMETER_STRUCT(FCitySampleSensorGridBvhTraversalCs, FGlobalShader);

public:
	static const uint32 ChunkSize = 32;
	static const uint32 SlackSize = 4;
	static const uint32 MinSensorCountLogTwo = 12;
	static const uint32 MaxSensorCountLogTwo = 14;

	class FMaxSensorCountLogTwo : SHADER_PERMUTATION_RANGE_INT("MAX_SENSOR_COUNT_LOG_TWO", MinSensorCountLogTwo, MaxSensorCountLogTwo - MinSensorCountLogTwo + 1);
	using FPermutationDomain = TShaderPermutationDomain<FMaxSensorCountLogTwo>;

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, CITYSAMPLESENSORGRIDSHADERS_API)
		SHADER_PARAMETER_SRV(Buffer<uint>, SensorCounts)
		SHADER_PARAMETER_SRV(Buffer<float4>, SensorLocations)
		SHADER_PARAMETER_SRV(StructuredBuffer<FInternalNodes>, InternalNodes)
		SHADER_PARAMETER_UAV(StructuredBuffer<FSensorInfo>, NearestSensors)
		SHADER_PARAMETER(float, MaxDistance)
		SHADER_PARAMETER(uint32, MaxSensorsPerOwner)
		SHADER_PARAMETER(uint32, PaddedSensorCount)
		SHADER_PARAMETER(uint32, PaddedInternalNodeCount)
		SHADER_PARAMETER(uint32, OwnerCount)
		SHADER_PARAMETER(uint32, SensorGridFactor)
	END_SHADER_PARAMETER_STRUCT()

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_CS"), 1);
		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_CHUNK_SIZE"), ChunkSize);
		OutEnvironment.SetDefine(TEXT("BVH_TRAVERSAL_STACK_SLACK_SIZE"), SlackSize);
	}
};
IMPLEMENT_GLOBAL_SHADER(FCitySampleSensorGridBvhTraversalCs, "/CitySampleSensorGrid/CitySampleSensorGridBvh.usf", "BvhTraversal", SF_Compute);

uint32 FCitySampleSensorGridHelper::GetMaxSensorDensity()
{
	return 1 << (FCitySampleSensorGridBvhTraversalCs::MaxSensorCountLogTwo / 2);
}

uint32 FCitySampleSensorGridHelper::GetMaxOwnerCount()
{
	return (1 << CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner) - 1;
}

FCitySampleSensorGridHelper::FCitySampleSensorGridHelper(ERHIFeatureLevel::Type InFeatureLevel, const FUintVector4& InSensorGridDimensions, uint32 FrameIndex)
	: FeatureLevel(InFeatureLevel)
	, SensorGridDimensions(InSensorGridDimensions)
{
}

FCitySampleSensorGridHelper::~FCitySampleSensorGridHelper()
{
}

bool FCitySampleSensorGridHelper::FTransientResources::Supports(const FResourceSizingInfo& OtherSizingInfo) const
{
	return OtherSizingInfo.SensorCount <= SizingInfo.SensorCount && OtherSizingInfo.OwnerCount <= SizingInfo.OwnerCount;
}

void FCitySampleSensorGridHelper::FTransientResources::Reset()
{
	PartialBounds = nullptr;
	LeafIndices[0] = nullptr;
	LeafIndices[1] = nullptr;
	MortonCodes[0] = nullptr;
	MortonCodes[1] = nullptr;
	DuplicateCounts = nullptr;
	CopyCommands = nullptr;
	ParentIndices = nullptr;
	HierarchyGates = nullptr;
	OwnerBoundingBoxes = nullptr;
	InternalNodes = nullptr;
	SensorCounts = nullptr;
	HasBuffers = false;
}

void FCitySampleSensorGridHelper::FTransientResources::Build(const FResourceSizingInfo& InSizingInfo)
{
	SizingInfo = InSizingInfo;

	if (SizingInfo.SensorCount <= 1)
	{
		Reset();
		return;
	}

	const uint32 InternalNodesPerOwner = SizingInfo.SensorCount - 1;
	const uint32 ParentsPerOwner = SizingInfo.SensorCount + InternalNodesPerOwner;
	const uint32 AlignedSensorsPerOwner = Align(InSizingInfo.SensorCount, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
	const uint32 DuplicateCountBlocksPerOwner = FMath::DivideAndRoundUp(AlignedSensorsPerOwner, CitySampleSensorGridShaders::MortonCompactionBufferSize);
	const uint32 DuplicateCountBlocks = DuplicateCountBlocksPerOwner * SizingInfo.OwnerCount;
	const uint32 IntermediaryBoundsPerOwner = FMath::DivideAndRoundUp(InSizingInfo.SensorCount, FCitySampleSensorGridBvhPrimeBoundsCs::ChunkSize);

	PartialBounds = MakeUnique<FRWBuffer>();
	PartialBounds->Initialize(
		TEXT("CitySampleSensorGridBvhPartialBounds"),
		sizeof(FVector4f),
		Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize) * SizingInfo.OwnerCount * 2,
		EPixelFormat::PF_A32B32G32R32F,
		ERHIAccess::UAVCompute,
		BUF_Static);

	for (TUniquePtr<FRWBuffer>& LeafIndicesBuffer : LeafIndices)
	{
		LeafIndicesBuffer = MakeUnique<FRWBuffer>();
		LeafIndicesBuffer->Initialize(
			TEXT("CitySampleSensorGridBvhLeafIndicesSorting"),
			sizeof(uint32),
			AlignedSensorsPerOwner * SizingInfo.OwnerCount,
			EPixelFormat::PF_R32_UINT,
			ERHIAccess::UAVCompute,
			BUF_Static);
	}

	for (TUniquePtr<FRWBuffer>& MortonCodesBuffer : MortonCodes)
	{
		MortonCodesBuffer = MakeUnique<FRWBuffer>();
		MortonCodesBuffer->Initialize(
			TEXT("CitySampleSensorGridBvhMortonCodesSorting"),
			sizeof(uint32),
			AlignedSensorsPerOwner * SizingInfo.OwnerCount,
			EPixelFormat::PF_R32_UINT,
			ERHIAccess::UAVCompute,
			BUF_Static);
	}

	DuplicateCounts = MakeUnique<FRWBuffer>();
	DuplicateCounts->Initialize(
		TEXT("CitySampleSensorGridBvhDuplicateCounts"),
		sizeof(uint32),
		DuplicateCountBlocks,
		EPixelFormat::PF_R32_UINT,
		ERHIAccess::UAVCompute,
		BUF_Static);

	CopyCommands = MakeUnique<FRWBuffer>();
	CopyCommands->Initialize(
		TEXT("CitySampleSensorGridBvhCopyCommands"),
		sizeof(FUintVector4),
		DuplicateCountBlocks,
		EPixelFormat::PF_R32G32B32A32_UINT,
		ERHIAccess::UAVCompute,
		BUF_Static);

	ParentIndices = MakeUnique<FRWBuffer>();
	ParentIndices->Initialize(
		TEXT("CitySampleSensorGridBvhParentIndices"),
		sizeof(uint32),
		Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount,
		EPixelFormat::PF_R32_UINT,
		ERHIAccess::UAVCompute,
		BUF_Static);

	HierarchyGates = MakeUnique<FRWBuffer>();
	HierarchyGates->Initialize(
		TEXT("CitySampleSensorGridBvhAccumGates"),
		sizeof(uint32),
		Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount,
		EPixelFormat::PF_R32_UINT,
		ERHIAccess::UAVCompute,
		BUF_Static);

	// buffer to store the bounds for each of the owners
	OwnerBoundingBoxes = MakeUnique<FRWBuffer>();
	OwnerBoundingBoxes->Initialize(
		TEXT("CitySampleSensorGridBvhOwnerBounds"),
		sizeof(FVector4f),
		SizingInfo.OwnerCount * 2,
		EPixelFormat::PF_A32B32G32R32F,
		ERHIAccess::UAVCompute,
		BUF_Static);
	
	InternalNodes = MakeUnique<FRWBufferStructured>();
	InternalNodes->Initialize(
		TEXT("CitySampleSensorGridBvhInternalNodes"),
		sizeof(CitySampleSensorGridShaders::FInternalNode),
		Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment) * SizingInfo.OwnerCount,
		BUF_Static,
		false /*bUseUavCounter*/,
		false /*bAppendBuffer*/,
		ERHIAccess::UAVCompute);

	SensorCounts = MakeUnique<FRWBuffer>();
	SensorCounts->Initialize(
		TEXT("CitySampleSensorGridBvhSensorCounts"),
		sizeof(uint32),
		SizingInfo.OwnerCount,
		EPixelFormat::PF_R32_UINT,
		ERHIAccess::UAVCompute,
		BUF_Static);

	HasBuffers = true;
}

void FCitySampleSensorGridHelper::FTransientResources::ResetTransitions(FRHICommandList& RHICmdList)
{
	if (HasBuffers)
	{
		RHICmdList.Transition({
			FRHITransitionInfo(PartialBounds->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(LeafIndices[0]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(LeafIndices[1]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(MortonCodes[0]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(MortonCodes[1]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(DuplicateCounts->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(CopyCommands->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(ParentIndices->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(HierarchyGates->UAV, ERHIAccess::UAVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(OwnerBoundingBoxes->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(InternalNodes->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
			FRHITransitionInfo(SensorCounts->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute)
		});
	}
}

void FCitySampleSensorGridHelper::ResetLeafBounds(
	FRHICommandList& RHICmdList,
	FRHIUnorderedAccessView* SensorLocationsUav)
{
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_ResetLeafBounds);

	TShaderMapRef<FCitySampleSensorGridResetSensorLocationsCs> ResetBoundsShader(GetGlobalShaderMap(FeatureLevel));
	FRHIComputeShader* ShaderRHI = ResetBoundsShader.GetComputeShader();

	SetComputePipelineState(RHICmdList, ShaderRHI);

	FCitySampleSensorGridResetSensorLocationsCs::FParameters PassParameters;
	PassParameters.SensorsToReset = SensorLocationsUav;
	PassParameters.SensorCount = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y) * SensorGridDimensions.Z;

	SetShaderParameters(RHICmdList, ResetBoundsShader, ShaderRHI, PassParameters);
	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(PassParameters.SensorCount, FCitySampleSensorGridResetSensorLocationsCs::ChunkSize), 1, 1);

	UnsetShaderUAVs(RHICmdList, ResetBoundsShader, ShaderRHI);
}

void FCitySampleSensorGridHelper::ResetResults(
	FRHICommandList& RHICmdList,
	FRHIUnorderedAccessView* ResultsUav)
{
	TShaderMapRef<FCitySampleSensorGridClearResultsCs> ClearResultsShader(GetGlobalShaderMap(FeatureLevel));
	FRHIComputeShader* ShaderRHI = ClearResultsShader.GetComputeShader();

	FCitySampleSensorGridClearResultsCs::FParameters PassParameters;
	PassParameters.NearestSensors = ResultsUav;
	PassParameters.SensorCount = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y) * SensorGridDimensions.Z;

	SetComputePipelineState(RHICmdList, ShaderRHI);
	SetShaderParameters(RHICmdList, ClearResultsShader, ShaderRHI, PassParameters);
	RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(PassParameters.SensorCount, FCitySampleSensorGridResetSensorLocationsCs::ChunkSize), 1, 1);
	UnsetShaderUAVs(RHICmdList, ClearResultsShader, ShaderRHI);

	RHICmdList.Transition(FRHITransitionInfo(ResultsUav, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
}

void FCitySampleSensorGridHelper::GenerateOwnerBounds(
	FRHICommandList& RHICmdList,
	FTransientResources& TransientResources,
	FRHIShaderResourceView* SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_GenerateOwnerBounds);

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 IntermediaryBoundsPerOwner = FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhPrimeBoundsCs::ChunkSize);

	// PrimeBounds
	{
		TShaderMapRef<FCitySampleSensorGridBvhPrimeBoundsCs> PrimeBoundsShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = PrimeBoundsShader.GetComputeShader();

		FCitySampleSensorGridBvhPrimeBoundsCs::FParameters PassParameters;

		PassParameters.SensorLocations = SensorLocationsSrv;
		PassParameters.PartialBoundingBoxes = TransientResources.PartialBounds->UAV;
		PassParameters.SensorCount = SensorsPerOwner;
		PassParameters.PaddedIntermediateCount = Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize);

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, PrimeBoundsShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(IntermediaryBoundsPerOwner, SensorGridDimensions.Z, 1);
		UnsetShaderUAVs(RHICmdList, PrimeBoundsShader, ShaderRHI);
	}

	RHICmdList.Transition(FRHITransitionInfo(TransientResources.PartialBounds->UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute));

	// FinalizeBounds
	{
		TShaderMapRef<FCitySampleSensorGridBvhFinalizeBoundsCs> FinalizeBoundsShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = FinalizeBoundsShader.GetComputeShader();

		FCitySampleSensorGridBvhFinalizeBoundsCs::FParameters PassParameters;

		PassParameters.SourceBoundingBoxes = TransientResources.PartialBounds->SRV;
		PassParameters.TargetBoundingBoxes = TransientResources.OwnerBoundingBoxes->UAV;
		PassParameters.SourceBoundsCount = IntermediaryBoundsPerOwner;
		PassParameters.PaddedSourceBoundsCount = Align(IntermediaryBoundsPerOwner, FCitySampleSensorGridBvhFinalizeBoundsCs::ChunkSize);

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, FinalizeBoundsShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(SensorGridDimensions.Z, 1, 1);
		UnsetShaderUAVs(RHICmdList, FinalizeBoundsShader, ShaderRHI);
	}

	RHICmdList.Transition(FRHITransitionInfo(TransientResources.OwnerBoundingBoxes->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute));
}

void FCitySampleSensorGridHelper::GenerateSortedLeaves(
	FRHICommandList& RHICmdList,
	FTransientResources& TransientResources,
	FRHIShaderResourceView* SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_GenerateSortedLeaves);

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;
	const uint32 AlignedSensorsPerOwner = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

	// for compaction we'll collapse each set of sensors per owner
	check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % CitySampleSensorGridShaders::MortonCompactionBufferSize) == 0);

	const uint32 DuplicateCountBlocksPerOwner = FMath::DivideAndRoundUp(AlignedSensorsPerOwner, CitySampleSensorGridShaders::MortonCompactionBufferSize);
	const uint32 DuplicateCountBlocks = DuplicateCountBlocksPerOwner * SensorGridDimensions.Z;

	// Generate morton codes
	{
		check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % FCitySampleSensorGridBvhMortonCs::ChunkSize) == 0);

		TShaderMapRef<FCitySampleSensorGridBvhMortonCs> MortonShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = MortonShader.GetComputeShader();

		FCitySampleSensorGridBvhMortonCs::FParameters PassParameters;

		PassParameters.SensorLocations = SensorLocationsSrv;
		PassParameters.BoundingBoxes = TransientResources.OwnerBoundingBoxes->SRV;
		PassParameters.LeafIndices = TransientResources.LeafIndices[1]->UAV;
		PassParameters.MortonCodes = TransientResources.MortonCodes[0]->UAV;
		PassParameters.SensorCount = SensorsPerOwner;
		PassParameters.PaddedOutputCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.OwnerBitCount = CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner;

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, MortonShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhMortonCs::ChunkSize), SensorGridDimensions.Z, 1);
		UnsetShaderUAVs(RHICmdList, MortonShader, ShaderRHI);
	}

	RHICmdList.Transition({
		FRHITransitionInfo(TransientResources.MortonCodes[0]->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.LeafIndices[1]->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute)
	});

	// GPUSort!
	{
		FGPUSortBuffers SortBuffers;
		SortBuffers.RemoteKeySRVs[0] = TransientResources.MortonCodes[0]->SRV;
		SortBuffers.RemoteKeySRVs[1] = TransientResources.MortonCodes[1]->SRV;

		SortBuffers.RemoteKeyUAVs[0] = TransientResources.MortonCodes[0]->UAV;
		SortBuffers.RemoteKeyUAVs[1] = TransientResources.MortonCodes[1]->UAV;

		SortBuffers.RemoteValueSRVs[0] = TransientResources.LeafIndices[1]->SRV;
		SortBuffers.RemoteValueSRVs[1] = TransientResources.LeafIndices[0]->SRV;

		SortBuffers.RemoteValueUAVs[0] = TransientResources.LeafIndices[1]->UAV;
		SortBuffers.RemoteValueUAVs[1] = TransientResources.LeafIndices[0]->UAV;

		SortBuffers.FirstValuesSRV = nullptr;
		SortBuffers.FinalValuesUAV = nullptr;

		int32 BufferIndex = 0;
		uint32 KeyMask = 0xFFFFFFFF;
		int32 Count = SensorsPerOwner * SensorGridDimensions.Z;

		SortGPUBuffers(RHICmdList, SortBuffers, BufferIndex, KeyMask, Count, FeatureLevel);
	}

	RHICmdList.Transition(FRHITransitionInfo(TransientResources.MortonCodes[1]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute));

	// go through the sorted buffer and find (and get rid of) duplicates
	{
		TShaderMapRef<FCitySampleSensorGridMortonCompactionCs> CompactShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = CompactShader.GetComputeShader();

		FCitySampleSensorGridMortonCompactionCs::FParameters PassParameters;
		PassParameters.InputValues = TransientResources.MortonCodes[0]->SRV;
		PassParameters.OutputValues = TransientResources.MortonCodes[1]->UAV;
		PassParameters.DuplicateCounts = TransientResources.DuplicateCounts->UAV;
		PassParameters.ValueCount = SensorsPerOwner * SensorGridDimensions.Z;
		PassParameters.OwnerBitCount = CitySampleSensorGridShaders::MortonCodeBitsReservedForOwner;

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, CompactShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(DuplicateCountBlocks, 1, 1);
		UnsetShaderUAVs(RHICmdList, CompactShader, ShaderRHI);
	}

	RHICmdList.Transition({
		FRHITransitionInfo(TransientResources.MortonCodes[1]->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.DuplicateCounts->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute)
	});

	{
		TShaderMapRef<FCitySampleSensorGridBuildCopyCommandsCs> BuildCommandsShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = BuildCommandsShader.GetComputeShader();

		FCitySampleSensorGridBuildCopyCommandsCs::FParameters PassParameters;
		PassParameters.DuplicateCounts = TransientResources.DuplicateCounts->SRV;
		PassParameters.CompactedValues = TransientResources.MortonCodes[1]->SRV;
		PassParameters.CopyCommands = TransientResources.CopyCommands->UAV;
		PassParameters.ElementsPerOwner = TransientResources.SensorCounts->UAV;
		PassParameters.OwnerCount = SensorGridDimensions.Z;
		PassParameters.GroupsPerOwner = DuplicateCountBlocksPerOwner;
		PassParameters.MaxElementsPerGroup = CitySampleSensorGridShaders::MortonCompactionBufferSize;

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, BuildCommandsShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(SensorGridDimensions.Z, 1, 1);
		UnsetShaderUAVs(RHICmdList, BuildCommandsShader, ShaderRHI);
	}

	RHICmdList.Transition({
		FRHITransitionInfo(TransientResources.CopyCommands->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.SensorCounts->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.MortonCodes[0]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute),
		FRHITransitionInfo(TransientResources.LeafIndices[0]->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute)
	});

	{
		TShaderMapRef<FCitySampleSensorGridShuffleDataCs> ShuffleShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = ShuffleShader.GetComputeShader();

		SetComputePipelineState(RHICmdList, ShaderRHI);

		// shuffle the morton codes
		{
			FCitySampleSensorGridShuffleDataCs::FParameters PassParameters;
			PassParameters.InputValues = TransientResources.MortonCodes[1]->SRV;
			PassParameters.CopyCommands = TransientResources.CopyCommands->SRV;
			PassParameters.OutputValues = TransientResources.MortonCodes[0]->UAV;

			SetShaderParameters(RHICmdList, ShuffleShader, ShaderRHI, PassParameters);
			RHICmdList.DispatchComputeShader(DuplicateCountBlocks, 1, 1);
		}

		// shuffle the indices
		{
			FCitySampleSensorGridShuffleDataCs::FParameters PassParameters;
			PassParameters.InputValues = TransientResources.LeafIndices[1]->SRV;
			PassParameters.CopyCommands = TransientResources.CopyCommands->SRV;
			PassParameters.OutputValues = TransientResources.LeafIndices[0]->UAV;

			SetShaderParameters(RHICmdList, ShuffleShader, ShaderRHI, PassParameters);
			RHICmdList.DispatchComputeShader(DuplicateCountBlocks, 1, 1);
		}

		UnsetShaderUAVs(RHICmdList, ShuffleShader, ShaderRHI);
	}

	RHICmdList.Transition({
		FRHITransitionInfo(TransientResources.MortonCodes[0]->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.LeafIndices[0]->UAV, ERHIAccess::UAVCompute, ERHIAccess::SRVCompute)
	});
}

void FCitySampleSensorGridHelper::GenerateBvh(
	FRHICommandList& RHICmdList,
	FTransientResources& TransientResources,
	FRHIShaderResourceView* SensorLocationsSrv)
{
	check(TransientResources.HasBuffers);
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_GenerateBvh);

	const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
	const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;
	const uint32 ParentsPerOwner = SensorsPerOwner + InternalNodesPerOwner;

	// top down pass for generating node relationships
	{
		check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % FCitySampleSensorGridBvhGenTopDownCs::ChunkSize) == 0);

		TShaderMapRef<FCitySampleSensorGridBvhGenTopDownCs> TopDownShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = TopDownShader.GetComputeShader();

		FCitySampleSensorGridBvhGenTopDownCs::FParameters PassParameters;
		PassParameters.LeafCounts = TransientResources.SensorCounts->SRV;
		PassParameters.LeafIndices = TransientResources.LeafIndices[0]->SRV;
		PassParameters.MortonCodes = TransientResources.MortonCodes[0]->SRV;
		PassParameters.InternalNodes = TransientResources.InternalNodes->UAV;
		PassParameters.ParentIndices = TransientResources.ParentIndices->UAV;
		PassParameters.AccumulationGates = TransientResources.HierarchyGates->UAV;
		PassParameters.InternalNodeParentOffset = SensorsPerOwner;
		PassParameters.PaddedLeafNodeCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.PaddedParentCount = Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, TopDownShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(InternalNodesPerOwner, FCitySampleSensorGridBvhGenTopDownCs::ChunkSize), SensorGridDimensions.Z, 1);
		UnsetShaderUAVs(RHICmdList, TopDownShader, ShaderRHI);
	}

	RHICmdList.Transition({
		FRHITransitionInfo(TransientResources.ParentIndices->UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute),
		FRHITransitionInfo(TransientResources.InternalNodes->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute),
		FRHITransitionInfo(TransientResources.HierarchyGates->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute)
	});

	// bottom up pass for completing bounds generation
	{
		check((CitySampleSensorGridShaders::SensorsPerOwnerAlignment % FCitySampleSensorGridBvhGenBottomUpCs::ChunkSize) == 0);

		TShaderMapRef<FCitySampleSensorGridBvhGenBottomUpCs> BottomUpShader(GetGlobalShaderMap(FeatureLevel));
		FRHIComputeShader* ShaderRHI = BottomUpShader.GetComputeShader();

		FCitySampleSensorGridBvhGenBottomUpCs::FParameters PassParameters;
		PassParameters.SensorCounts = TransientResources.SensorCounts->SRV;
		PassParameters.SensorLocations = SensorLocationsSrv;
		PassParameters.ParentIndices = TransientResources.ParentIndices->SRV;
		PassParameters.InternalNodes = TransientResources.InternalNodes->UAV;
		PassParameters.AccumulationGates = TransientResources.HierarchyGates->UAV;
		PassParameters.InternalNodeParentOffset = SensorsPerOwner;
		PassParameters.MaxSensorsPerOwner = SensorsPerOwner;
		PassParameters.PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.PaddedParentCount = Align(ParentsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, BottomUpShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhGenBottomUpCs::ChunkSize), SensorGridDimensions.Z, 1);
		UnsetShaderUAVs(RHICmdList, BottomUpShader, ShaderRHI);
	}

	RHICmdList.Transition(FRHITransitionInfo(TransientResources.InternalNodes->UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute));
}

void FCitySampleSensorGridHelper::RunTraversals(
	FRHICommandList& RHICmdList,
	const FVector2D& GlobalSensorRange,
	FTransientResources& TransientResources,
	FRHIShaderResourceView* SensorLocationsSrv,
	FRHIUnorderedAccessView* ResultsUav)
{
	check(TransientResources.HasBuffers);
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_RunTraversals);

	{
		const uint32 SensorsPerOwner = (1 << SensorGridDimensions.X) * (1 << SensorGridDimensions.Y);
		const uint32 SensorsPerOwnerLogTwo = FMath::Max(FCitySampleSensorGridBvhTraversalCs::MinSensorCountLogTwo, FMath::CeilLogTwo(SensorsPerOwner));
		const uint32 InternalNodesPerOwner = SensorsPerOwner - 1;

		if (!ensure(SensorsPerOwnerLogTwo <= FCitySampleSensorGridBvhTraversalCs::MaxSensorCountLogTwo))
		{
			return;
		}

		FCitySampleSensorGridBvhTraversalCs::FPermutationDomain PermutationVector;
		PermutationVector.Set<FCitySampleSensorGridBvhTraversalCs::FMaxSensorCountLogTwo>(SensorsPerOwnerLogTwo);

		TShaderMapRef<FCitySampleSensorGridBvhTraversalCs> TraversalShader(GetGlobalShaderMap(FeatureLevel), PermutationVector);
		FRHIComputeShader* ShaderRHI = TraversalShader.GetComputeShader();

		FCitySampleSensorGridBvhTraversalCs::FParameters PassParameters;
		PassParameters.SensorCounts = TransientResources.SensorCounts->SRV;
		PassParameters.SensorLocations = SensorLocationsSrv;
		PassParameters.InternalNodes = TransientResources.InternalNodes->SRV;
		PassParameters.NearestSensors = ResultsUav;
		PassParameters.MaxDistance = GlobalSensorRange.Y;
		PassParameters.MaxSensorsPerOwner = SensorsPerOwner;
		PassParameters.PaddedSensorCount = Align(SensorsPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.PaddedInternalNodeCount = Align(InternalNodesPerOwner, CitySampleSensorGridShaders::SensorsPerOwnerAlignment);
		PassParameters.OwnerCount = SensorGridDimensions.Z;
		PassParameters.SensorGridFactor = SensorGridDimensions.X;

		SetComputePipelineState(RHICmdList, ShaderRHI);
		SetShaderParameters(RHICmdList, TraversalShader, ShaderRHI, PassParameters);
		RHICmdList.DispatchComputeShader(FMath::DivideAndRoundUp(SensorsPerOwner, FCitySampleSensorGridBvhTraversalCs::ChunkSize), SensorGridDimensions.Z, 1);
		UnsetShaderUAVs(RHICmdList, TraversalShader, ShaderRHI);
	}
}

void FCitySampleSensorGridHelper::NearestSensors(
	FRHICommandList& RHICmdList,
	const FVector2D& GlobalSensorRange,
	FTransientResources& TransientResources,
	FRHIShaderResourceView* SensorLocationsSrv,
	FRHIUnorderedAccessView* ResultsUav)
{
	SCOPED_DRAW_EVENT(RHICmdList, CitySampleSensorGrid_BvhNearestSensors);

	ResetResults(RHICmdList, ResultsUav);

	if (TransientResources.SizingInfo.OwnerCount > 1 && TransientResources.HasBuffers && !GCitySampleSensorGridBuildDisabled)
	{
		GenerateOwnerBounds(RHICmdList, TransientResources, SensorLocationsSrv);
		GenerateSortedLeaves(RHICmdList, TransientResources, SensorLocationsSrv);
		GenerateBvh(RHICmdList, TransientResources, SensorLocationsSrv);
		RunTraversals(RHICmdList, GlobalSensorRange, TransientResources, SensorLocationsSrv, ResultsUav);
	}
}
