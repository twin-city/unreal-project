// Copyright Epic Games, Inc. All Rights Reserved.

#include "MassTrafficDriverVisualizationTrait.h"
#include "MassTrafficFragments.h"
#include "MassTrafficSubsystem.h"

#include "MassEntityTemplateRegistry.h"
#include "MassRepresentationSubsystem.h"
#include "Animation/AnimClassInterface.h"
#include "MassEntityUtils.h"


void UMassTrafficDriverVisualizationTrait::BuildTemplate(FMassEntityTemplateBuildContext& BuildContext, const UWorld& World) const 
{
	FMassEntityManager& EntityManager = UE::Mass::Utils::GetEntityManagerChecked(World);

	UMassRepresentationSubsystem* RepresentationSubsystem = Cast<UMassRepresentationSubsystem>(World.GetSubsystemBase(RepresentationSubsystemClass));
	check(RepresentationSubsystem);
	
	BuildContext.AddFragment<FMassTrafficDriverVisualizationFragment>();

	// Make a mutable copy of Params so we can register the driver meshes and assign the description IDs
	FMassTrafficDriversParameters RegisteredParams = Params;
	if (IsValid(RegisteredParams.DriverTypesData))
	{
		for (const FMassTrafficDriverTypeData& DriverType : RegisteredParams.DriverTypesData->DriverTypes)
		{
			// Register visual types
			FStaticMeshInstanceVisualizationDesc StaticMeshInstanceVisualizationDesc;
			StaticMeshInstanceVisualizationDesc.Meshes.Reserve(DriverType.Meshes.Num());
			for (const FMassTrafficDriverMesh& DriverMesh : DriverType.Meshes)
			{
				StaticMeshInstanceVisualizationDesc.Meshes.Add(FStaticMeshInstanceVisualizationMeshDesc({DriverMesh.StaticMesh, DriverMesh.MaterialOverrides, DriverMesh.MinLODSignificance, DriverMesh.MaxLODSignificance, false/*bCastShadow*/, EComponentMobility::Movable}));
			}
			int16 DriverTypeStaticMeshDescIndex = RepresentationSubsystem->FindOrAddStaticMeshDesc(StaticMeshInstanceVisualizationDesc);
			RegisteredParams.DriverTypesStaticMeshDescIndex.Add(DriverTypeStaticMeshDescIndex);
		}
	}

	// Register & add shared fragment
	const FConstSharedStruct ParamsSharedFragment = EntityManager.GetOrCreateConstSharedFragment(RegisteredParams);
	BuildContext.AddConstSharedFragment(ParamsSharedFragment);
}

UMassTrafficDriverInitializer::UMassTrafficDriverInitializer()
	: EntityQuery(*this)
{
	ObservedType = FMassTrafficDriverVisualizationFragment::StaticStruct();
	Operation = EMassObservedOperation::Add;
}

void UMassTrafficDriverInitializer::Initialize(UObject& Owner)
{
	Super::Initialize(Owner);

	// Seed RandomStream
	const int32 TrafficRandomSeed = GetDefault<UMassTrafficSettings>()->RandomSeed;
	if (TrafficRandomSeed >= 0)
	{
		RandomStream.Initialize(TrafficRandomSeed);
	}
	else
	{
		RandomStream.GenerateNewSeed();
	}
}

void UMassTrafficDriverInitializer::ConfigureQueries()
{
	EntityQuery.AddConstSharedRequirement<FMassTrafficDriversParameters>();
	EntityQuery.AddRequirement<FMassTrafficDriverVisualizationFragment>(EMassFragmentAccess::ReadWrite);
}

void UMassTrafficDriverInitializer::Execute(FMassEntityManager& EntitySubsystem,
	FMassExecutionContext& Context)
{
	// Generate random fractions 
	EntityQuery.ForEachEntityChunk(EntitySubsystem, Context, [this](FMassExecutionContext& QueryContext)
	{
		// Get driver types
		const FMassTrafficDriversParameters& Params = QueryContext.GetConstSharedFragment<FMassTrafficDriversParameters>();
		const int32 NumDriverTypes = IsValid(Params.DriverTypesData) ? Params.DriverTypesData->DriverTypes.Num() : 0;
		check(NumDriverTypes <= FMassTrafficDriverVisualizationFragment::InvalidDriverTypeIndex);

		// If no driver types defined, leave DriverTypeIndex at its default InvalidDriverTypeIndex
		if (NumDriverTypes == 0)
		{
			return;
		}
		
		const TArrayView<FMassTrafficDriverVisualizationFragment> DriverVisualizationFragments = QueryContext.GetMutableFragmentView<FMassTrafficDriverVisualizationFragment>();
		for (FMassTrafficDriverVisualizationFragment& DriverVisualizationFragment : DriverVisualizationFragments)
		{
			// Assign a random driver type
			DriverVisualizationFragment.DriverTypeIndex = RandomStream.RandHelper(NumDriverTypes);
		}
	});
}

