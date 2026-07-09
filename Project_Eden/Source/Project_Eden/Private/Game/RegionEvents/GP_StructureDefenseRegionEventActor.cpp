#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"

#include "Components/StaticMeshComponent.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "UObject/ConstructorHelpers.h"

AGP_StructureDefenseRegionEventActor::AGP_StructureDefenseRegionEventActor()
{
	DefenseStructureMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DefenseStructureMesh"));
	DefenseStructureMesh->SetupAttachment(GetRootComponent());
	DefenseStructureMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	DefenseStructureMesh->SetCollisionObjectType(ECC_WorldDynamic);
	DefenseStructureMesh->SetCollisionResponseToAllChannels(ECR_Block);
	DefenseStructureMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 2.4f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StructureMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (StructureMeshFinder.Succeeded())
	{
		DefenseStructureMesh->SetStaticMesh(StructureMeshFinder.Object);
	}
}

void AGP_StructureDefenseRegionEventActor::ActivateRegionEvent()
{
	if (!HasAuthority() || GetRuntimeState() != EGPRegionEventRuntimeState::Dormant || !IsValid(GetEventData()))
	{
		return;
	}

	if (GetEventData()->bApplyActiveRegionState)
	{
		ApplyRegionState(GetEventData()->ActiveRegionState);
	}

	SetRuntimeState(EGPRegionEventRuntimeState::Active);

	if (bSpawnInitialDefenseWave)
	{
		SpawnDefenseWave();
	}

	if (UWorld* World = GetWorld())
	{
		DefenseEndWorldTimeSeconds = World->GetTimeSeconds() + FMath::Max(1.0f, DefenseDurationSeconds);
		World->GetTimerManager().SetTimer(
			DefenseCompleteTimerHandle,
			this,
			&ThisClass::CompleteRegionEvent,
			FMath::Max(1.0f, DefenseDurationSeconds),
			false);

		World->GetTimerManager().SetTimer(
			DefenseWaveTimerHandle,
			this,
			&ThisClass::SpawnDefenseWave,
			FMath::Max(0.1f, DefenseWaveIntervalSeconds),
			true,
			FMath::Max(0.1f, DefenseWaveIntervalSeconds));
	}
}

void AGP_StructureDefenseRegionEventActor::CompleteRegionEvent()
{
	StopDefenseTimers();
	Super::CompleteRegionEvent();
}

void AGP_StructureDefenseRegionEventActor::ExpireRegionEvent()
{
	StopDefenseTimers();
	Super::ExpireRegionEvent();
}

float AGP_StructureDefenseRegionEventActor::GetRemainingDefenseTime() const
{
	const UWorld* World = GetWorld();
	return World ? FMath::Max(0.0f, DefenseEndWorldTimeSeconds - World->GetTimeSeconds()) : 0.0f;
}

void AGP_StructureDefenseRegionEventActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDefenseTimers();
	Super::EndPlay(EndPlayReason);
}

void AGP_StructureDefenseRegionEventActor::SpawnDefenseWave()
{
	if (HasAuthority() && GetRuntimeState() == EGPRegionEventRuntimeState::Active)
	{
		// Defense waves use the same event data spawn list as other combat events.
		SpawnConfiguredEnemies();
	}
}

void AGP_StructureDefenseRegionEventActor::StopDefenseTimers()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DefenseCompleteTimerHandle);
		World->GetTimerManager().ClearTimer(DefenseWaveTimerHandle);
	}
}
