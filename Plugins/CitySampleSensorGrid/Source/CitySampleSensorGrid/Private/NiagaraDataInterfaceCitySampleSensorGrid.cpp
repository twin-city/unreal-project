// Copyright Epic Games, Inc. All Rights Reserved.

#include "NiagaraDataInterfaceCitySampleSensorGrid.h"

#include "CitySampleSensorGridShaders.h"
#include "GlobalDistanceFieldParameters.h"
#include "NiagaraComponent.h"
#include "NiagaraTypes.h"
#include "NiagaraWorldManager.h"
#include "RenderResource.h"
#include "Shader.h"
#include "ShaderCore.h"
#include "ShaderCompilerCore.h"
#include "ShaderParameterUtils.h"
#include "NiagaraGpuComputeDispatchInterface.h"

static float GCitySampleSensorGridRadiusOverride = -1.0f;
static FAutoConsoleVariableRef CVarCitySampleSensorGridRadiusOverride(
	TEXT("CitySample.sensorgrid.RadiusOverride"),
	GCitySampleSensorGridRadiusOverride,
	TEXT("When > 0, this will override any CitySampleSensorGrid DI settings"),
	ECVF_Default
);

// c++ mirror of the struct defined in CitySampleSensorGridCommon.ush
struct alignas(16) FSensorInfo
{
	FVector Location;
	uint32 DistanceUint;
	FIntVector HitIndex;
	uint32 SearchCount;
};

#define LOCTEXT_NAMESPACE "NiagaraDataInterfaceCitySampleSensorGrid"

namespace NDICitySampleSensorGridLocal
{
	static const TCHAR* CommonShaderFile = TEXT("/CitySampleSensorGrid/NiagaraDataInterfaceCitySampleSensorGrid.ush");
	static const TCHAR* TemplateShaderFile = TEXT("/CitySampleSensorGrid/NiagaraDataInterfaceCitySampleSensorGridTemplate.ush");

	static const FName UpdateSensorName(TEXT("UpdateSensorGpu"));
	static const FText UpdateSensorDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UpdateSensorDescription", "Updates the sensor location, should be called each frame"), FText());

	static const FName FindNearestName(TEXT("FindNearestGpu"));
	static const FText FindNearestDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("FindNearestDescription", "Grabs information on the closest sensor (from a different grid) from the previous frame"), FText());

	static const FName ReadUserChannelName(TEXT("ReadSensorUserChannel"));
	static const FText ReadUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("ReadUserChannelDescription", "Reads a user data channel from the previous frame"), FText());

	static const FName WriteUserChannelName(TEXT("WriteSensorUserChannel"));
	static const FText WriteUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("WriteUserChannelDescription", "Sets a user data channel, should be called each frame"), FText());

	static const FString SensorLocationsParamName(TEXT("SensorLocations_"));
	static const FString SensorInfoParamName(TEXT("SensorInfo_"));
	static const FString SensorGridDimensionsParamName(TEXT("SensorGridDimensions_"));
	static const FString SensorGridWriteIndexParamName(TEXT("SensorGridWriteIndex_"));
	static const FString SensorGridReadIndexParamName(TEXT("SensorGridReadIndex_"));

	static const FString SensorGridUserChannelCountParamName(TEXT("SensorGridUserChannelCount_"));
	static const FString SensorGridUserChannelDataParamName(TEXT("SensorGridUserChannelData_"));
	static const FString SensorGridPreviousUserChannelDataParamName(TEXT("SensorGridPreviousUserChannelData_"));

	enum NodeVersion : int32
	{
		NodeVersion_Initial = 0,
		Nodeversion_Reworked_Output,
		NodeVersion_Readded_SensorOutput,

		VersionPlusOne,
		NodeVersion_Latest = VersionPlusOne - 1,
	};

	static const FText SensorXDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorXDescription", "X coordinate of the sensor"), FText());
	static const FText SensorYDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorYDescription", "Y coordinate of the sensor"), FText());
	static const FText OwnerIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OwnerIndexDescription", "Identifier of the grid the sensor belongs to"), FText());
	static const FText LocationDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("LocationDescription", "Position in 3D space"), FText());
	static const FText SensorRangeDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorRangeDescription", "Experimental - Per sensor radius (additive to the global search radius)"), FText());
	static const FText SensorValidDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("SensorValidDescription", "Indicates that the sensor is findable from other grids"), FText());
	static const FText UserChannelIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UserChannelIndexDescription", "Index of the user channel"), FText());
	static const FText UserChannelValueDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("UserChannelValueDescription", "Value of the user channel"), FText());

	static const FText OutputLocationDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputLocationDescription", "Location of the closest sensor from the previous frame"), FText());
	static const FText OutputDistanceDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputDistanceDescription", "Distance to the closest sensor from the previous frame"), FText());
	static const FText OutputIsValidDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputIsValidDescription", "Indicates whether a valid sensor was found the previous frame"), FText());
	static const FText OutputSensorXDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputSensorXDescription", "X coordinate of the closest sensor from the previous frame"), FText());
	static const FText OutputSensorYDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputSensorYdDescription", "Y coordinate of the closest sensor from the previous frame"), FText());
	static const FText OutputOwnerIndexDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputOwnerIndexDescription", "Identifier of the grid associatd with the closest sensor from the previous frame - not guaranteed to be stable"), FText());
	static const FText OutputUserChannelDescription = IF_WITH_EDITORONLY_DATA(LOCTEXT("OutputUserChanneldDescription", "Value of the user channel from the previous frame"), FText());
}

struct FSensorGridNetworkProxy
{
	// RWBuffer used to hold the sensor locations
	//	-populated by simulation pass
	//	-aggregated into the hierarchy through a global shader
	TUniquePtr<FRWBuffer> SensorLocations;

	// StructuredBuffer storing the results of a global query for finding the closest sensor
	//	-populated by global shader
	//	-read by simulation pass of the system instances
	TUniquePtr<FRWBufferStructured> SensorInfo;

	TUniquePtr<FRWBufferStructured> EmptyResults;
	TUniquePtr<FRWBuffer> DummyUserChannel;

	TUniquePtr<FRWBuffer> UserChannelData;
	TUniquePtr<FRWBuffer> PreviousUserChannelData;

	TUniquePtr<FCitySampleSensorGridHelper::FTransientResources> TransientResources;

	// mapping between the system instance ID and the subnetwork within the buffers
	TMap<FNiagaraSystemInstanceID, int32> InstanceOwnerReadIndexMap;
	TMap<FNiagaraSystemInstanceID, int32> InstanceOwnerWriteIndexMap;

	TSet<FNiagaraSystemInstanceID> RegisteredInstances;

	int32 QueuedOwnerCount = 0;
	int32 AllocatedOwnerCount = 0;
	int32 ResultsOwnerCount = 0;

	// XY is the grid of sensors
	const uint32 SensorGridLayerCount;

	// Number of float4 channels to reserve for each sensor
	const int32 UserChannelCount;

	FSensorGridNetworkProxy() = delete;
	FSensorGridNetworkProxy(uint32 SensorGridWidth, int32 InUserChannelCount);

	void PrepareSimulation(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type InFeatureLevel, bool ClearEachFrame);
	void EndSimulation(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type InFeatureLevel, const FVector2D& GlobalSensorRange);
};

struct FNiagaraDataIntefaceProxyCitySampleSensorGrid : public FNiagaraDataInterfaceProxy
{
	FRWLock NetworkLock;
	TMap<const FNiagaraGpuComputeDispatchInterface*, TUniquePtr<FSensorGridNetworkProxy>> NetworkProxies;

	uint32 SensorGridSize = 0;
	FVector2D GlobalSensorRange = FVector2D(EForceInit::ForceInitToZero);
	int32 UserChannelCount = 0;
	bool ClearEachFrame = false;

	virtual int32 PerInstanceDataPassedToRenderThreadSize() const override;
	virtual void PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context) override;
	virtual bool RequiresPreStageFinalize() const override;
	virtual void FinalizePreStage(FRHICommandList& RHICmdList, const FNiagaraGpuComputeDispatchInterface* Batcher) override;
	virtual bool RequiresPostStageFinalize() const override;
	virtual void FinalizePostStage(FRHICommandList& RHICmdList, const FNiagaraGpuComputeDispatchInterface* Batcher) override;

	FSensorGridNetworkProxy* GetNetwork(const FNiagaraGpuComputeDispatchInterface* Batcher);

	void RegisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID);
	void UnregisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID);

	void RenderThreadInitialize(int32 InSensorCount, const FVector2D& InGlobalSensorRange, bool InClearEachFrame, int32 InUserChannelCount);
};


UNiagaraDataInterfaceCitySampleSensorGrid::UNiagaraDataInterfaceCitySampleSensorGrid(FObjectInitializer const& ObjectInitializer)
	: Super(ObjectInitializer)
{
    Proxy.Reset(new FNiagaraDataIntefaceProxyCitySampleSensorGrid());
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PostInitProperties()
{
	Super::PostInitProperties();

	//Can we register data interfaces as regular types and fold them into the FNiagaraVariable framework for UI and function calls etc?
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ENiagaraTypeRegistryFlags Flags = ENiagaraTypeRegistryFlags::AllowAnyVariable | ENiagaraTypeRegistryFlags::AllowParameter;
		FNiagaraTypeRegistry::Register(FNiagaraTypeDefinition(GetClass()), Flags);
	}
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PostLoad()
{
	Super::PostLoad();

	if (SensorCountPerSide)
	{
		MarkRenderDataDirty();
	}
}

void UNiagaraDataInterfaceCitySampleSensorGrid::GetFunctions(TArray<FNiagaraFunctionSignature>& OutFunctions)
{
	{
		FNiagaraFunctionSignature SigUpdateSensor;
		SigUpdateSensor.Name = NDICitySampleSensorGridLocal::UpdateSensorName;
		SigUpdateSensor.bRequiresExecPin = true;
		SigUpdateSensor.bMemberFunction = true;
		SigUpdateSensor.bRequiresContext = false;
		SigUpdateSensor.bSupportsCPU = false;
		#if WITH_EDITORONLY_DATA
		SigUpdateSensor.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigUpdateSensor.Description = NDICitySampleSensorGridLocal::UpdateSensorDescription;
		#endif

		SigUpdateSensor.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Location")), NDICitySampleSensorGridLocal::LocationDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("SensorRange")), NDICitySampleSensorGridLocal::SensorRangeDescription);
		SigUpdateSensor.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("IsValid")), NDICitySampleSensorGridLocal::SensorValidDescription);
		OutFunctions.Add(SigUpdateSensor);
	}

	{
		FNiagaraFunctionSignature SigFindNearest;
		SigFindNearest.Name = NDICitySampleSensorGridLocal::FindNearestName;
		SigFindNearest.bMemberFunction = true;
		SigFindNearest.bRequiresContext = false;
		SigFindNearest.bSupportsCPU = false;
		#if WITH_EDITORONLY_DATA
		SigFindNearest.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigFindNearest.Description = NDICitySampleSensorGridLocal::FindNearestDescription;
		#endif

		SigFindNearest.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigFindNearest.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigFindNearest.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Out_Location")), NDICitySampleSensorGridLocal::OutputLocationDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Out_Distance")), NDICitySampleSensorGridLocal::OutputDistanceDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorX")),
		NDICitySampleSensorGridLocal::OutputSensorXDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorY")),
		NDICitySampleSensorGridLocal::OutputSensorYDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_OwnerIndex")),
		NDICitySampleSensorGridLocal::OutputOwnerIndexDescription);
		SigFindNearest.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
		OutFunctions.Add(SigFindNearest);
	}

	{
		FNiagaraFunctionSignature SigReadUserData;
		SigReadUserData.Name = NDICitySampleSensorGridLocal::ReadUserChannelName;
		SigReadUserData.bMemberFunction = true;
		SigReadUserData.bRequiresContext = false;
		SigReadUserData.bSupportsCPU = false;
		#if WITH_EDITORONLY_DATA
		SigReadUserData.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigReadUserData.Description = NDICitySampleSensorGridLocal::ReadUserChannelDescription;
		#endif

		SigReadUserData.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("OwnerIndex")),
		NDICitySampleSensorGridLocal::OwnerIndexDescription);
		SigReadUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Channel")), NDICitySampleSensorGridLocal::UserChannelIndexDescription);
		SigReadUserData.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Out_UserData")), NDICitySampleSensorGridLocal::OutputUserChannelDescription);
		SigReadUserData.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
		OutFunctions.Add(SigReadUserData);
	}

	{
		FNiagaraFunctionSignature SigWriteUserData;
		SigWriteUserData.Name = NDICitySampleSensorGridLocal::WriteUserChannelName;
		SigWriteUserData.bRequiresExecPin = true;
		SigWriteUserData.bMemberFunction = true;
		SigWriteUserData.bRequiresContext = false;
		SigWriteUserData.bSupportsCPU = false;
		#if WITH_EDITORONLY_DATA
		SigWriteUserData.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;
		SigWriteUserData.Description = NDICitySampleSensorGridLocal::WriteUserChannelDescription;
		#endif

		SigWriteUserData.Inputs.Add(FNiagaraVariable(FNiagaraTypeDefinition(GetClass()), TEXT("CitySampleSensorGrid")));
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_X")), NDICitySampleSensorGridLocal::SensorXDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Sensor_Y")), NDICitySampleSensorGridLocal::SensorYDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Channel")), NDICitySampleSensorGridLocal::UserChannelIndexDescription);
		SigWriteUserData.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec4Def(), TEXT("Value")), NDICitySampleSensorGridLocal::UserChannelValueDescription);
		OutFunctions.Add(SigWriteUserData);
	}
}

#if WITH_EDITORONLY_DATA
bool UNiagaraDataInterfaceCitySampleSensorGrid::GetFunctionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, const FNiagaraDataInterfaceGeneratedFunction& FunctionInfo, int FunctionInstanceIndex, FString& OutHLSL)
{
	if ( (FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::UpdateSensorName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::FindNearestName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::ReadUserChannelName) ||
		(FunctionInfo.DefinitionName == NDICitySampleSensorGridLocal::WriteUserChannelName))
	{
		return true;
	}

	return false;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::GetParameterDefinitionHLSL(const FNiagaraDataInterfaceGPUParamInfo& ParamInfo, FString& OutHLSL)
{
	TMap<FString, FStringFormatArg> TemplateArgs =
	{
		{TEXT("ParameterName"),	ParamInfo.DataInterfaceHLSLSymbol},
	};

	FString TemplateFile;
	LoadShaderSourceFile(NDICitySampleSensorGridLocal::TemplateShaderFile, EShaderPlatform::SP_PCD3D_SM5, &TemplateFile, nullptr);
	OutHLSL += FString::Format(*TemplateFile, TemplateArgs);
}

void UNiagaraDataInterfaceCitySampleSensorGrid::GetCommonHLSL(FString& OutHlsl)
{
	OutHlsl.Appendf(TEXT("#include \"%s\"\n"), NDICitySampleSensorGridLocal::CommonShaderFile);
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::AppendCompileHash(FNiagaraCompileHashVisitor* InVisitor) const
{
	if (!Super::AppendCompileHash(InVisitor))
	{
		return false;
	}

	InVisitor->UpdateString(TEXT("NDICitySampleSensorGridCommonHLSLSource"), GetShaderFileHash(NDICitySampleSensorGridLocal::CommonShaderFile, EShaderPlatform::SP_PCD3D_SM5).ToString());
	InVisitor->UpdateString(TEXT("NDICitySampleSensorGridTemplateHLSLSource"), GetShaderFileHash(NDICitySampleSensorGridLocal::TemplateShaderFile, EShaderPlatform::SP_PCD3D_SM5).ToString());

	return true;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::ModifyCompilationEnvironment(EShaderPlatform ShaderPlatform, FShaderCompilerEnvironment& OutEnvironment) const
{
	Super::ModifyCompilationEnvironment(ShaderPlatform, OutEnvironment);
}


bool UNiagaraDataInterfaceCitySampleSensorGrid::UpgradeFunctionCall(FNiagaraFunctionSignature& FunctionSignature)
{
	bool WasChanged = false;

	if (FunctionSignature.Name == NDICitySampleSensorGridLocal::UpdateSensorName && FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::NodeVersion_Initial)
	{
		FunctionSignature.AddInput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("IsValid")), NDICitySampleSensorGridLocal::SensorValidDescription);
		FunctionSignature.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;

		WasChanged = true;
	}

	if (FunctionSignature.Name == NDICitySampleSensorGridLocal::FindNearestName)
	{
		if (FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::NodeVersion_Initial
			|| FunctionSignature.FunctionVersion == NDICitySampleSensorGridLocal::NodeVersion::Nodeversion_Reworked_Output)
		{
			FunctionSignature.Outputs.Reset();
			FunctionSignature.OutputDescriptions.Reset();

			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Out_Location")), NDICitySampleSensorGridLocal::OutputLocationDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Out_Distance")), NDICitySampleSensorGridLocal::OutputDistanceDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorX")),
			NDICitySampleSensorGridLocal::OutputSensorXDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_SensorY")),
			NDICitySampleSensorGridLocal::OutputSensorYDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetIntDef(), TEXT("Out_OwnerIndex")),
			NDICitySampleSensorGridLocal::OutputOwnerIndexDescription);
			FunctionSignature.AddOutput(FNiagaraVariable(FNiagaraTypeDefinition::GetBoolDef(), TEXT("Out_IsValid")), NDICitySampleSensorGridLocal::OutputIsValidDescription);
			FunctionSignature.FunctionVersion = NDICitySampleSensorGridLocal::NodeVersion_Latest;

			WasChanged = true;
		}
	}

	return WasChanged;
}

#endif

int32 UNiagaraDataInterfaceCitySampleSensorGrid::PerInstanceDataSize() const
{
	return 4;
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::InitPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	if (SystemInstance)
	{
		FNiagaraDataIntefaceProxyCitySampleSensorGrid* RT_Proxy = GetProxyAs<FNiagaraDataIntefaceProxyCitySampleSensorGrid>();

		// Push Updates to Proxy
		ENQUEUE_RENDER_COMMAND(FRegisterInstance)(
			[RT_Proxy, RT_Batcher = SystemInstance->GetComputeDispatchInterface(), RT_InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->RegisterNetworkInstance(RT_Batcher, RT_InstanceID);
		});
	}

	return true;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::DestroyPerInstanceData(void* PerInstanceData, FNiagaraSystemInstance* SystemInstance)
{
	if (SystemInstance)
	{
		FNiagaraDataIntefaceProxyCitySampleSensorGrid* RT_Proxy = GetProxyAs<FNiagaraDataIntefaceProxyCitySampleSensorGrid>();

		// Push Updates to Proxy
		ENQUEUE_RENDER_COMMAND(FUnregisterInstance)(
			[RT_Proxy, RT_Batcher = SystemInstance->GetComputeDispatchInterface(), RT_InstanceID = SystemInstance->GetId()](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->UnregisterNetworkInstance(RT_Batcher, RT_InstanceID);
		});
	}
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::Equals(const UNiagaraDataInterface* Other) const
{
	if (!Super::Equals(Other))
	{
		return false;
	}

	const UNiagaraDataInterfaceCitySampleSensorGrid* OtherTyped = CastChecked<const UNiagaraDataInterfaceCitySampleSensorGrid>(Other);
	return OtherTyped->SensorCountPerSide == SensorCountPerSide
		&& OtherTyped->GlobalSensorAccuracy == GlobalSensorAccuracy
		&& OtherTyped->GlobalSensorRange == GlobalSensorRange
		&& OtherTyped->ClearEachFrame == ClearEachFrame
		&& OtherTyped->UserChannelCount == UserChannelCount;
}

bool UNiagaraDataInterfaceCitySampleSensorGrid::CopyToInternal(UNiagaraDataInterface* Destination) const
{
	if (!Super::CopyToInternal(Destination))
	{
		return false;
	}

	UNiagaraDataInterfaceCitySampleSensorGrid* OtherTyped = CastChecked<UNiagaraDataInterfaceCitySampleSensorGrid>(Destination);
	OtherTyped->SensorCountPerSide = SensorCountPerSide;
	OtherTyped->GlobalSensorAccuracy = GlobalSensorAccuracy;
	OtherTyped->GlobalSensorRange = GlobalSensorRange;
	OtherTyped->ClearEachFrame = ClearEachFrame;
	OtherTyped->UserChannelCount = UserChannelCount;
	OtherTyped->MarkRenderDataDirty();
	return true;
}

void UNiagaraDataInterfaceCitySampleSensorGrid::PushToRenderThreadImpl()
{
	FNiagaraDataIntefaceProxyCitySampleSensorGrid* RT_Proxy = GetProxyAs<FNiagaraDataIntefaceProxyCitySampleSensorGrid>();

	// Push Updates to Proxy, first release any resources
	ENQUEUE_RENDER_COMMAND(FUpdateDI)(
		[RT_Proxy,
		RT_SensorCount = SensorCountPerSide,
		RT_GlobalSensorRange = FVector2D(GlobalSensorAccuracy, GlobalSensorRange),
		RT_ClearEachFrame = ClearEachFrame,
		RT_UserChannelCount = UserChannelCount](FRHICommandListImmediate& RHICmdList)
		{
			RT_Proxy->RenderThreadInitialize(RT_SensorCount, RT_GlobalSensorRange, RT_ClearEachFrame, RT_UserChannelCount);
		});
}

#if WITH_EDITOR
void UNiagaraDataInterfaceCitySampleSensorGrid::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		(PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, SensorCountPerSide)
		|| PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, GlobalSensorAccuracy)
		|| PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UNiagaraDataInterfaceCitySampleSensorGrid, GlobalSensorRange)))
	{
		MarkRenderDataDirty();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////

FSensorGridNetworkProxy::FSensorGridNetworkProxy(uint32 InSensorGridLayerCount, int32 InUserChannelCount)
	: SensorGridLayerCount(InSensorGridLayerCount)
	, UserChannelCount(InUserChannelCount)
{
	// initialize with a dummy SensorInfo structured buffer which will be used for the frames where we don't have any results
	EmptyResults = MakeUnique<FRWBufferStructured>();
	EmptyResults->Initialize(
		TEXT("CitySampleSensorGridEmptyResults"),
		sizeof(FSensorInfo),
		1,
		BUF_Static,
		false /*bUseUavCounter*/,
		false /*bAppendBuffer*/,
		ERHIAccess::SRVCompute);

	DummyUserChannel = MakeUnique<FRWBuffer>();
	DummyUserChannel->Initialize(
		TEXT("CitySampleSensorGridDummyyUserChannel"),
		sizeof(FVector4f),
		1,
		EPixelFormat::PF_A32B32G32R32F,
		ERHIAccess::UAVCompute,
		BUF_Static);

	TransientResources = MakeUnique<FCitySampleSensorGridHelper::FTransientResources>();
}

void FSensorGridNetworkProxy::PrepareSimulation(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type InFeatureLevel, bool ClearEachFrame)
{
	if (!QueuedOwnerCount)
	{
		SensorLocations = nullptr;
		UserChannelData = nullptr;
		PreviousUserChannelData = nullptr;
	}
	else
	{
		FCitySampleSensorGridHelper::FResourceSizingInfo SizingInfo;
		SizingInfo.SensorCount = (1 << SensorGridLayerCount) * (1 << SensorGridLayerCount);
		SizingInfo.OwnerCount = QueuedOwnerCount;

		if (QueuedOwnerCount != AllocatedOwnerCount)
		{
			int32 HierarchySensorCount = 0;
			for (uint32 LevelIt = 0; LevelIt < SensorGridLayerCount; ++LevelIt)
			{
				const uint32 SensorCount = 1 << LevelIt;
				HierarchySensorCount += SensorCount * SensorCount * QueuedOwnerCount;
			}

			SensorLocations = MakeUnique<FRWBuffer>();
			SensorLocations->Initialize(
				TEXT("CitySampleSensorGridLocations"),
				sizeof(FVector4f),
				SizingInfo.SensorCount * QueuedOwnerCount,
				EPixelFormat::PF_A32B32G32R32F,
				ERHIAccess::UAVCompute,
				BUF_Static);

			if (!TransientResources->Supports(SizingInfo))
			{
				TransientResources->Build(SizingInfo);
			}
		}

		if (UserChannelCount)
		{
			TUniquePtr<FRWBuffer> TempBuffer = MoveTemp(PreviousUserChannelData);
			PreviousUserChannelData = MoveTemp(UserChannelData);

			if (QueuedOwnerCount != ResultsOwnerCount || !TempBuffer)
			{
				UserChannelData = MakeUnique<FRWBuffer>();
				UserChannelData->Initialize(
					TEXT("CitySampleSensorGridUserChannelData"),
					sizeof(FVector4f),
					SizingInfo.SensorCount * QueuedOwnerCount * UserChannelCount,
					EPixelFormat::PF_A32B32G32R32F,
					ERHIAccess::UAVCompute,
					BUF_Static);
			}
			else
			{
				UserChannelData = MoveTemp(TempBuffer);
			}
		}
	}

	AllocatedOwnerCount = QueuedOwnerCount;
	QueuedOwnerCount = 0;

	if (AllocatedOwnerCount)
	{
		RHICmdList.BeginUAVOverlap(SensorLocations->UAV);

		FCitySampleSensorGridHelper Helper(InFeatureLevel, FUintVector4(SensorGridLayerCount, SensorGridLayerCount, AllocatedOwnerCount, 0), GFrameNumberRenderThread);

		if (ClearEachFrame)
		{
			RHICmdList.Transition(FRHITransitionInfo(SensorLocations->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
			Helper.ResetLeafBounds(RHICmdList, SensorLocations->UAV);
			RHICmdList.Transition(FRHITransitionInfo(SensorLocations->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
		}

		if (UserChannelCount)
		{
			RHICmdList.BeginUAVOverlap(UserChannelData->UAV);
		}
	}
}

void FSensorGridNetworkProxy::EndSimulation(FRHICommandList& RHICmdList, ERHIFeatureLevel::Type InFeatureLevel, const FVector2D& GlobalSensorRange)
{
	if (AllocatedOwnerCount)
	{
		const uint32 SensorCountPerSide = 1 << SensorGridLayerCount;

		FVector2D SensorRange = GlobalSensorRange;
		if (GCitySampleSensorGridRadiusOverride > 0.0f)
		{
			SensorRange.Y = GCitySampleSensorGridRadiusOverride;
		}

		SCOPED_DRAW_EVENT(RHICmdList, FNiagaraDataIntefaceProxyCitySampleSensorGrid_PostSimulate);
		if (AllocatedOwnerCount != ResultsOwnerCount)
		{
			ResultsOwnerCount = AllocatedOwnerCount;

			SensorInfo = MakeUnique<FRWBufferStructured>();
			SensorInfo->Initialize(
				TEXT("CitySampleSensorGridSensorInfo"),
				sizeof(FSensorInfo),
				SensorCountPerSide * SensorCountPerSide * ResultsOwnerCount,
				BUF_Static,
				false /*bUseUavCounter*/,
				false /*bAppendBuffer*/,
				ERHIAccess::SRVCompute);
		}

		RHICmdList.EndUAVOverlap(SensorLocations->UAV);
		if (UserChannelCount)
		{
			RHICmdList.EndUAVOverlap(UserChannelData->UAV);
		}

		FCitySampleSensorGridHelper Helper(InFeatureLevel, FUintVector4(SensorGridLayerCount, SensorGridLayerCount, AllocatedOwnerCount, 0), GFrameNumberRenderThread);

		RHICmdList.Transition({
			FRHITransitionInfo(SensorLocations->UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute),
			FRHITransitionInfo(SensorInfo->UAV, ERHIAccess::SRVCompute, ERHIAccess::UAVCompute)
		});

		Helper.NearestSensors(RHICmdList, SensorRange, *TransientResources, SensorLocations->SRV, SensorInfo->UAV);

		RHICmdList.Transition(FRHITransitionInfo(SensorInfo->UAV, ERHIAccess::Unknown, ERHIAccess::SRVCompute));

		InstanceOwnerReadIndexMap = InstanceOwnerWriteIndexMap;
		InstanceOwnerWriteIndexMap.Reset();

		TransientResources->ResetTransitions(RHICmdList);
	}
	else
	{
		SensorInfo = nullptr;
	}
}

int32 FNiagaraDataIntefaceProxyCitySampleSensorGrid::PerInstanceDataPassedToRenderThreadSize() const
{
	return 0;
}

FSensorGridNetworkProxy* FNiagaraDataIntefaceProxyCitySampleSensorGrid::GetNetwork(const FNiagaraGpuComputeDispatchInterface* Batcher)
{
	FReadScopeLock _Scope(NetworkLock);
	return NetworkProxies.FindChecked(Batcher).Get();
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::RegisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID)
{
	FWriteScopeLock _Scope(NetworkLock);
	TUniquePtr<FSensorGridNetworkProxy>& Network = NetworkProxies.FindOrAdd(Batcher, nullptr);
	if (!Network)
	{
		Network = MakeUnique<FSensorGridNetworkProxy>(SensorGridSize, UserChannelCount);
	}

	Network->RegisteredInstances.Add(SystemInstanceID);
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::UnregisterNetworkInstance(const FNiagaraGpuComputeDispatchInterface* Batcher, FNiagaraSystemInstanceID SystemInstanceID)
{
	FWriteScopeLock _Scope(NetworkLock);
	if (TUniquePtr<FSensorGridNetworkProxy>* NetworkPtr = NetworkProxies.Find(Batcher))
	{
		if (FSensorGridNetworkProxy* Network = NetworkPtr->Get())
		{
			Network->RegisteredInstances.Remove(SystemInstanceID);
			if (!Network->RegisteredInstances.Num())
			{
				NetworkProxies.Remove(Batcher);
			}
		}
	}
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::PreStage(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceStageArgs& Context)
{
	FNiagaraDataInterfaceProxy::PreStage(RHICmdList, Context);

	if (FSensorGridNetworkProxy* Network = GetNetwork(Context.ComputeDispatchInterface))
	{
		int32& OwnerIndex = Network->InstanceOwnerWriteIndexMap.FindOrAdd(Context.SystemInstanceID, INDEX_NONE);
		if (OwnerIndex == INDEX_NONE)
		{
			if (ensure(Network->QueuedOwnerCount < ((int32)FCitySampleSensorGridHelper::GetMaxOwnerCount())))
			{
				OwnerIndex = Network->QueuedOwnerCount++;
			}
		}
	}
}

bool FNiagaraDataIntefaceProxyCitySampleSensorGrid::RequiresPreStageFinalize() const
{
	return true;
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::FinalizePreStage(FRHICommandList& RHICmdList, const FNiagaraGpuComputeDispatchInterface* Batcher)
{
	FNiagaraDataInterfaceProxy::FinalizePreStage(RHICmdList, Batcher);

	if (FSensorGridNetworkProxy* Network = GetNetwork(Batcher))
	{
		Network->PrepareSimulation(RHICmdList, Batcher->GetFeatureLevel(), ClearEachFrame);
	}
}

bool FNiagaraDataIntefaceProxyCitySampleSensorGrid::RequiresPostStageFinalize() const
{
	return true;
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::FinalizePostStage(FRHICommandList& RHICmdList, const FNiagaraGpuComputeDispatchInterface* Batcher)
{
	FNiagaraDataInterfaceProxy::FinalizePostStage(RHICmdList, Batcher);

	if (FSensorGridNetworkProxy* Network = GetNetwork(Batcher))
	{
		Network->EndSimulation(RHICmdList, Batcher->GetFeatureLevel(), GlobalSensorRange);
	}
}

void FNiagaraDataIntefaceProxyCitySampleSensorGrid::RenderThreadInitialize(int32 InSensorCount, const FVector2D& InGlobalSensorRange, bool InClearEachFrame, int32 InUserChannelCount)
{
	// SensorCount needs to be a power of two
	SensorGridSize = FMath::Min(FCitySampleSensorGridHelper::GetMaxSensorDensity(), FMath::CeilLogTwo(InSensorCount));
	GlobalSensorRange.X = FMath::Max(0.0f, InGlobalSensorRange.X);
	GlobalSensorRange.Y = FMath::Max(GlobalSensorRange.X, InGlobalSensorRange.Y);
	ClearEachFrame = InClearEachFrame;
	UserChannelCount = InUserChannelCount;

	NetworkProxies.Reset();
}

struct FNiagaraDataInterfaceParametersCS_CitySampleSesnorGrid : public FNiagaraDataInterfaceParametersCS
{
	DECLARE_TYPE_LAYOUT(FNiagaraDataInterfaceParametersCS_CitySampleSesnorGrid, NonVirtual);
public:
	void Bind(const FNiagaraDataInterfaceGPUParamInfo& ParameterInfo, const class FShaderParameterMap& ParameterMap)
	{
		SensorLocationsParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorLocationsParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		SensorInfoParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorInfoParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		SensorGridDimensionsParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridDimensionsParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		SensorGridWriteIndexParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridWriteIndexParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		SensorGridReadIndexParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridReadIndexParamName + ParameterInfo.DataInterfaceHLSLSymbol));

		UserChannelCountParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridUserChannelCountParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		UserChannelDataParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridUserChannelDataParamName + ParameterInfo.DataInterfaceHLSLSymbol));
		PreviousUserChannelDataParam.Bind(ParameterMap, *(NDICitySampleSensorGridLocal::SensorGridPreviousUserChannelDataParamName + ParameterInfo.DataInterfaceHLSLSymbol));
	}

	void Set(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
	{
		check(IsInRenderingThread());

		FNiagaraDataIntefaceProxyCitySampleSensorGrid* Proxy = (FNiagaraDataIntefaceProxyCitySampleSensorGrid*)Context.DataInterface;
		FRHIComputeShader* ComputeShaderRHI = Context.Shader.GetComputeShader();

		FSensorGridNetworkProxy* NetworkProxy = Proxy->GetNetwork(Context.ComputeDispatchInterface);
		check(NetworkProxy);

		if (SensorLocationsParam.IsBound())
		{
			check(NetworkProxy->SensorLocations);
			RHICmdList.Transition(FRHITransitionInfo(NetworkProxy->SensorLocations->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
			RHICmdList.SetUAVParameter(ComputeShaderRHI, SensorLocationsParam.GetUAVIndex(), NetworkProxy->SensorLocations->UAV);
		}

		if (SensorInfoParam.IsBound())
		{
			FRHIShaderResourceView* SRV = NetworkProxy->EmptyResults->SRV;
			if (NetworkProxy->SensorInfo)
			{
				SRV = NetworkProxy->SensorInfo->SRV;
			}
			SetSRVParameter(RHICmdList, ComputeShaderRHI, SensorInfoParam, SRV);
		}

		SetShaderValue(RHICmdList, ComputeShaderRHI, SensorGridDimensionsParam, FUintVector4(NetworkProxy->SensorGridLayerCount, NetworkProxy->SensorGridLayerCount, NetworkProxy->AllocatedOwnerCount, 0));
		SetShaderValue(RHICmdList, ComputeShaderRHI, UserChannelCountParam, NetworkProxy->UserChannelCount);

		const bool HasUserChannels = NetworkProxy->UserChannelCount > 0;

		if (UserChannelDataParam.IsBound())
		{
			check(HasUserChannels);
			if (HasUserChannels)
			{
				RHICmdList.Transition(FRHITransitionInfo(NetworkProxy->UserChannelData->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
				RHICmdList.SetUAVParameter(ComputeShaderRHI, UserChannelDataParam.GetUAVIndex(), NetworkProxy->UserChannelData->UAV);
			}
			else
			{
				RHICmdList.Transition(FRHITransitionInfo(NetworkProxy->DummyUserChannel->UAV, ERHIAccess::Unknown, ERHIAccess::UAVCompute));
				RHICmdList.SetUAVParameter(ComputeShaderRHI, UserChannelDataParam.GetUAVIndex(), NetworkProxy->DummyUserChannel->UAV);
			}
		}

		if (PreviousUserChannelDataParam.IsBound())
		{
			check(HasUserChannels);
			if (HasUserChannels)
			{
				FRHIShaderResourceView* SRV = FNiagaraRenderer::GetDummyFloat4Buffer();
				if (NetworkProxy->PreviousUserChannelData)
				{
					SRV = NetworkProxy->PreviousUserChannelData->SRV;
				}
				SetSRVParameter(RHICmdList, ComputeShaderRHI, PreviousUserChannelDataParam, SRV);
			}
			else
			{
				SetSRVParameter(RHICmdList, ComputeShaderRHI, PreviousUserChannelDataParam, FNiagaraRenderer::GetDummyFloat4Buffer());
			}
		}

		{
			const int32* WriteIndex = NetworkProxy->InstanceOwnerWriteIndexMap.Find(Context.SystemInstanceID);
			SetShaderValue(RHICmdList, ComputeShaderRHI, SensorGridWriteIndexParam, WriteIndex ? *WriteIndex : INDEX_NONE);
		}

		{
			const int32* ReadIndex = NetworkProxy->InstanceOwnerReadIndexMap.Find(Context.SystemInstanceID);
			SetShaderValue(RHICmdList, ComputeShaderRHI, SensorGridReadIndexParam, ReadIndex ? *ReadIndex : INDEX_NONE);
		}
	}

	void Unset(FRHICommandList& RHICmdList, const FNiagaraDataInterfaceSetArgs& Context) const
	{
		FRHIComputeShader* ComputeShaderRHI = Context.Shader.GetComputeShader();
		FNiagaraDataIntefaceProxyCitySampleSensorGrid* Proxy = (FNiagaraDataIntefaceProxyCitySampleSensorGrid*)Context.DataInterface;

		if (SensorLocationsParam.IsUAVBound())
		{
			SensorLocationsParam.UnsetUAV(RHICmdList, ComputeShaderRHI);
		}

		if (UserChannelDataParam.IsUAVBound())
		{
			UserChannelDataParam.UnsetUAV(RHICmdList, ComputeShaderRHI);
		}
	}

private:
	LAYOUT_FIELD(FRWShaderParameter, SensorLocationsParam);
	LAYOUT_FIELD(FShaderResourceParameter, SensorInfoParam);
	LAYOUT_FIELD(FShaderParameter, SensorGridDimensionsParam);
	LAYOUT_FIELD(FShaderParameter, SensorGridWriteIndexParam);
	LAYOUT_FIELD(FShaderParameter, SensorGridReadIndexParam);

	// user channel data
	LAYOUT_FIELD(FShaderParameter, UserChannelCountParam);
	LAYOUT_FIELD(FRWShaderParameter, UserChannelDataParam);
	LAYOUT_FIELD(FShaderResourceParameter, PreviousUserChannelDataParam);
};

PRAGMA_DISABLE_DEPRECATION_WARNINGS

IMPLEMENT_TYPE_LAYOUT(FNiagaraDataInterfaceParametersCS_CitySampleSesnorGrid);

IMPLEMENT_NIAGARA_DI_PARAMETER(UNiagaraDataInterfaceCitySampleSensorGrid, FNiagaraDataInterfaceParametersCS_CitySampleSesnorGrid);

PRAGMA_ENABLE_DEPRECATION_WARNINGS

#undef LOCTEXT_NAMESPACE
