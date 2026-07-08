#include "Game/RegionEvents/GP_RegionEventDirector.h"

#include "Game/GP_EnemySpawnVolume.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventSelectionPolicy.h"

AGP_RegionEventDirector::AGP_RegionEventDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RegionEventActorClass = AGP_RegionEventActor::StaticClass();
}

void AGP_RegionEventDirector::BeginPlay()
{
	Super::BeginPlay();

	// Standalone placement still works even if GameMode forgets to pass RegionCount.
	if (HasAuthority() && !bDirectorInitialized)
	{
		InitializeRegionEventDirector(RegionCount);
	}
}

void AGP_RegionEventDirector::InitializeRegionEventDirector(int32 InRegionCount)
{
	if (!HasAuthority())
	{
		return;
	}

	RegionCount = FMath::Max(0, InRegionCount);
	EventRandomStream.Initialize(RandomSeed);
	bDirectorInitialized = true;
	RemoveInactiveEvents();
}

AGP_RegionEventActor* AGP_RegionEventDirector::TryStartRegionEventForZone(AGP_EnemySpawnVolume* Zone, EGPRegionEventTrigger Trigger)
{
	if (!HasAuthority() || !bEnableRegionEvents || !IsValid(Zone) || !*RegionEventActorClass)
	{
		return nullptr;
	}

	RemoveInactiveEvents();
	if (MaxActiveEvents >= 0 && ActiveEvents.Num() >= MaxActiveEvents)
	{
		return nullptr;
	}

	const int32 RegionId = ResolveRegionIdForZone(Zone);
	if (RegionId == INDEX_NONE || HasActiveEventForRegion(RegionId))
	{
		return nullptr;
	}

	const UGP_RegionEventData* SelectedEvent = GPRegionEventSelectionPolicy::SelectWeightedEvent(
		EventPool,
		Trigger,
		EventRandomStream);
	if (SelectedEvent == nullptr)
	{
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.Owner = this;

	AGP_RegionEventActor* EventActor = World->SpawnActor<AGP_RegionEventActor>(
		RegionEventActorClass,
		ResolveSpawnLocationForZone(Zone),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!IsValid(EventActor))
	{
		return nullptr;
	}

	EventActor->OnRegionEventStateChanged.AddDynamic(this, &ThisClass::HandleRegionEventStateChanged);
	EventActor->OnRegionEventEnemySpawned.AddDynamic(this, &ThisClass::HandleRegionEventEnemySpawned);
	EventActor->InitializeRegionEvent(RegionId, const_cast<UGP_RegionEventData*>(SelectedEvent));
	ActiveEvents.Add(EventActor);

	OnRegionEventStarted.Broadcast(this, EventActor);

	if (bAutoActivateSpawnedEvents)
	{
		EventActor->ActivateRegionEvent();
	}

	return EventActor;
}

void AGP_RegionEventDirector::CompleteEventsForRegion(int32 RegionId)
{
	if (!HasAuthority())
	{
		return;
	}

	for (AGP_RegionEventActor* EventActor : ActiveEvents)
	{
		if (IsValid(EventActor) && EventActor->GetRegionId() == RegionId)
		{
			EventActor->CompleteRegionEvent();
		}
	}

	RemoveInactiveEvents();
}

void AGP_RegionEventDirector::CompleteEventsForZone(AGP_EnemySpawnVolume* Zone)
{
	CompleteEventsForRegion(ResolveRegionIdForZone(Zone));
}

bool AGP_RegionEventDirector::HasActiveEventForRegion(int32 RegionId) const
{
	for (const AGP_RegionEventActor* EventActor : ActiveEvents)
	{
		if (IsValid(EventActor)
			&& EventActor->GetRegionId() == RegionId
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
		{
			return true;
		}
	}

	return false;
}

int32 AGP_RegionEventDirector::ResolveRegionIdForZone(const AGP_EnemySpawnVolume* Zone) const
{
	if (!IsValid(Zone))
	{
		return INDEX_NONE;
	}

	const TArray<int32>& RegionsToRevive = Zone->GetRegionsToRevive();
	if (RegionsToRevive.Num() > 0)
	{
		return RegionsToRevive[0];
	}

	// Boss/fallback zones often do not own a revival list; their order gives a stable presentation region.
	const int32 FallbackRegionId = Zone->GetZoneOrder();
	return RegionCount <= 0 || FallbackRegionId < RegionCount ? FallbackRegionId : INDEX_NONE;
}

FVector AGP_RegionEventDirector::ResolveSpawnLocationForZone(AGP_EnemySpawnVolume* Zone) const
{
	if (!IsValid(Zone))
	{
		return GetActorLocation() + SpawnOffset;
	}

	bool bProjected = false;
	const FVector ProjectedZoneLocation = Zone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);
	return (bProjected ? ProjectedZoneLocation : Zone->GetActorLocation()) + SpawnOffset;
}

void AGP_RegionEventDirector::RemoveInactiveEvents()
{
	for (int32 Index = ActiveEvents.Num() - 1; Index >= 0; --Index)
	{
		AGP_RegionEventActor* EventActor = ActiveEvents[Index];
		if (!IsValid(EventActor)
			|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Completed
			|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Expired)
		{
			ActiveEvents.RemoveAtSwap(Index);
		}
	}
}

void AGP_RegionEventDirector::HandleRegionEventStateChanged(AGP_RegionEventActor* EventActor)
{
	if (!IsValid(EventActor))
	{
		RemoveInactiveEvents();
		return;
	}

	if (EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Completed
		|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Expired)
	{
		OnRegionEventEnded.Broadcast(this, EventActor);

		// Keep the actor around briefly so Blueprint VFX can finish naturally.
		if (FinishedEventLifeSpan > 0.0f)
		{
			EventActor->SetLifeSpan(FinishedEventLifeSpan);
		}

		RemoveInactiveEvents();
	}
}

void AGP_RegionEventDirector::HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy)
{
	OnRegionEventEnemySpawned.Broadcast(EventActor, Enemy);
}
