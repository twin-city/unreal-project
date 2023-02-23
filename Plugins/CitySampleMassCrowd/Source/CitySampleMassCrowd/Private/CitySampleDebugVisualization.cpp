// Copyright Epic Games, Inc. All Rights Reserved.

#include "CitySampleDebugVisualization.h"

#include "MassCommonFragments.h"
#include "MassDebuggerSubsystem.h"
#include "MassRepresentationFragments.h"
#include "Engine/World.h"

namespace FCitySampleDebugVisualizationTraitHelper
{
	constexpr EMassEntityDebugShape LODToShapeMapping[] = {
		EMassEntityDebugShape::Capsule, // EMassLOD::High
		EMassEntityDebugShape::Cone, // EMassLOD::Medium
		EMassEntityDebugShape::Cylinder, // EMassLOD::Low
		EMassEntityDebugShape::Box, // EMassLOD::Off
		EMassEntityDebugShape::Box, // EMassLOD::Max
	};
	static_assert(sizeof(LODToShapeMapping) / sizeof(EMassEntityDebugShape) == int(EMassLOD::Max) + 1, "LODToShapeMapping must account for all EMassLOD values");
} // FCitySampleDebugVisualizationTraitHelper

//----------------------------------------------------------------------//
//  UCitySampleDebugVisProcessor
//----------------------------------------------------------------------//
UCitySampleDebugVisProcessor::UCitySampleDebugVisProcessor()
	: EntityQuery(*this)
{
	ExecutionOrder.ExecuteAfter.Add(UE::Mass::ProcessorGroupNames::UpdateWorldFromMass);
}

void UCitySampleDebugVisProcessor::ConfigureQueries()
{
	EntityQuery.AddRequirement<FMassRepresentationLODFragment>(EMassFragmentAccess::ReadOnly);
	EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
}

void UCitySampleDebugVisProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
	UMassDebuggerSubsystem* Debugger = UWorld::GetSubsystem<UMassDebuggerSubsystem>(GetWorld());
	if (Debugger == nullptr || !Debugger->IsCollectingData())
	{
		return;
	}

	QUICK_SCOPE_CYCLE_COUNTER(UCitySampleDebugVisProcessor_Run);

	EntityQuery.ForEachEntityChunk(EntityManager, Context, [=](FMassExecutionContext& Context)
		{
			const int32 NumEntities = Context.GetNumEntities();
			TConstArrayView<FTransformFragment> LocationList = Context.GetFragmentView<FTransformFragment>();
			TConstArrayView<FMassRepresentationLODFragment> RepresentationLODList = Context.GetFragmentView<FMassRepresentationLODFragment>();

			for (int32 i = 0; i < NumEntities; ++i)
			{
				const FMassRepresentationLODFragment& RepresentationLOD = RepresentationLODList[i];
				Debugger->AddShape(FCitySampleDebugVisualizationTraitHelper::LODToShapeMapping[int(RepresentationLOD.LOD)], LocationList[i].GetTransform().GetLocation(), AgentRadiusToUse);
			}
		});

	Debugger->DataCollected();
}