#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Game/Corruption/GP_CorruptionPresentationActor.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunPortal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Kismet/GameplayStatics.h"

AGP_GameMode::AGP_GameMode()
{
	GameStateClass = AGP_GameState::StaticClass();
	RegionEventDirectorClass = AGP_RegionEventDirector::StaticClass();
	CorruptionPresentationClass = AGP_CorruptionPresentationActor::StaticClass();

	// Match the lobby's seamless transition so arriving clients are carried
	// into this map instead of being dropped during the load.
	bUseSeamlessTravel = true;
}

void AGP_GameMode::BeginPlay()
{
	Super::BeginPlay();

	GatherZones();
	ResolveRegionEventDirector();
	SpawnCorruptionPresentation();

	// Let placed BP actors bind to GameState delegates in their BeginPlay before
	// the initial all-dead reset is broadcast on the server/listen host.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AGP_GameMode::InitializeRegionStates));
	}

	// Subscribe to all volumes; unlock zone 0 so it fires when the first player steps in.
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		Zone->OnPlayerEnteredZone.AddDynamic(this, &AGP_GameMode::HandlePlayerEnteredZone);
		Zone->OnMarkerTriggered.AddDynamic(this, &AGP_GameMode::HandleMarkerTriggered);
	}

	UnlockZone(0);
}

void AGP_GameMode::InitializeRegionStates()
{
	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->InitRegionStates(RegionCount, DeadRegionState);

		if (UGP_WorldCorruptionComponent* Corruption = GPGameState->GetWorldCorruptionComponent())
		{
			// The authoritative timer lives with GameState so seamless clients receive one shared progression value.
			Corruption->InitializeCorruption(
				RegionCount,
				bEnableWorldCorruption ? InitialCorruption : 0.0f,
				MaximumCorruption,
				PassiveCorruptionIncreasePerMinute,
				CorruptionUpdateInterval,
				bEnableWorldCorruption);
		}
	}
}

void AGP_GameMode::SpawnCorruptionPresentation()
{
	if (!bAutoSpawnCorruptionPresentation || !bEnableWorldCorruption || !*CorruptionPresentationClass)
	{
		return;
	}

	// Preserve an authored presentation actor or a seamless-travel carry-over instead of spawning a duplicate.
	if (UGameplayStatics::GetActorOfClass(this, AGP_CorruptionPresentationActor::StaticClass()))
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		World->SpawnActor<AGP_CorruptionPresentationActor>(
			CorruptionPresentationClass,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
	}
}

void AGP_GameMode::GatherZones()
{
	TArray<AActor*> FoundZones;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemySpawnVolume::StaticClass(), FoundZones);

	OrderedZones.Reset();
	for (AActor* FoundZone : FoundZones)
	{
		if (AGP_EnemySpawnVolume* Zone = Cast<AGP_EnemySpawnVolume>(FoundZone))
		{
			OrderedZones.Add(Zone);
		}
	}

	OrderedZones.Sort([](const AGP_EnemySpawnVolume& A, const AGP_EnemySpawnVolume& B)
	{
		return A.GetZoneOrder() < B.GetZoneOrder();
	});
}

void AGP_GameMode::ResolveRegionEventDirector()
{
	TArray<AActor*> FoundDirectors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_RegionEventDirector::StaticClass(), FoundDirectors);

	for (AActor* FoundActor : FoundDirectors)
	{
		if (AGP_RegionEventDirector* FoundDirector = Cast<AGP_RegionEventDirector>(FoundActor))
		{
			RegionEventDirector = FoundDirector;
			break;
		}
	}

	if (!RegionEventDirector && bAutoSpawnRegionEventDirector && *RegionEventDirectorClass)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			RegionEventDirector = World->SpawnActor<AGP_RegionEventDirector>(
				RegionEventDirectorClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
		}
	}

	if (RegionEventDirector)
	{
		// The director is server orchestration; spawned event actors replicate their own presentation state.
		RegionEventDirector->InitializeRegionEventDirector(RegionCount);
		RegionEventDirector->OnRegionEventEnemySpawned.AddDynamic(this, &ThisClass::HandleRegionEventEnemySpawned);
	}
}

void AGP_GameMode::UnlockZone(int32 ZoneIndex)
{
	if (!OrderedZones.IsValidIndex(ZoneIndex))
	{
		return;
	}

	PendingZoneIndex = ZoneIndex;
	OrderedZones[ZoneIndex]->Unlock();

	UE_LOG(LogTemp, Log, TEXT("[GP_GameMode] Zone %d unlocked — waiting for player to enter."), ZoneIndex);
}

void AGP_GameMode::HandlePlayerEnteredZone(AGP_EnemySpawnVolume* Zone)
{
	if (bRunFinished || !IsValid(Zone))
	{
		return;
	}

	const int32 ZoneIndex = OrderedZones.IndexOfByKey(Zone);
	if (ZoneIndex != PendingZoneIndex)
	{
		return;
	}

	StartZone(ZoneIndex);
}

void AGP_GameMode::StartRun()
{
	// Legacy manual trigger — unlocks and immediately starts zone 0 without waiting for overlap.
	if (bRunStarted)
	{
		return;
	}

	if (OrderedZones.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] No AGP_EnemySpawnVolume zones found in the level; nothing to run."));
		FinishRun(/*bVictory=*/true);
		return;
	}

	StartZone(0);
}

void AGP_GameMode::StartZone(int32 ZoneIndex)
{
	if (bRunFinished)
	{
		return;
	}

	if (!OrderedZones.IsValidIndex(ZoneIndex))
	{
		FinishRun(/*bVictory=*/true);
		return;
	}

	bRunStarted = true;
	CurrentZoneIndex = ZoneIndex;
	AliveZoneEnemies = 0;
	bHasLastDeathLocation = false;

	AGP_EnemySpawnVolume* Zone = OrderedZones[ZoneIndex];

	MarkersTotal = Zone->GetMarkerCount();
	MarkersTriggered = 0;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetCurrentZoneIndex(ZoneIndex);
		GPGameState->SetEnemiesRemaining(0);
		GPGameState->SetMatchPhase(Zone->IsBossZone() ? EGPMatchPhase::BossFight : EGPMatchPhase::InCity);
	}

	if (MarkersTotal > 0)
	{
		// Progressive mode: enemies spawn marker-by-marker as the player advances into the city.
		Zone->ActivateMarkers();
		StartRegionEventForZone(Zone, EGPRegionEventTrigger::ZoneStarted);
		OnZoneStarted(ZoneIndex, Zone);
		return;
	}

	// Box-fallback mode: spawn the whole composition at once.
	StartRegionEventForZone(Zone, EGPRegionEventTrigger::ZoneStarted);
	SpawnZoneEnemies(Zone);
	OnZoneStarted(ZoneIndex, Zone);

	if (AliveZoneEnemies == 0)
	{
		CompleteCurrentZone();
	}
}

void AGP_GameMode::HandleMarkerTriggered(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker)
{
	if (bRunFinished || !IsValid(Zone) || !Marker || OrderedZones.IndexOfByKey(Zone) != CurrentZoneIndex)
	{
		return;
	}

	++MarkersTriggered;
	SpawnMarkerEnemies(Zone, Marker);

	// A marker with no valid enemies still counts as triggered; if that empties the zone, complete it.
	MaybeCompleteZone();
}

void AGP_GameMode::SpawnMarkerEnemies(AGP_EnemySpawnVolume* Zone, AGP_EnemySpawnMarker* Marker)
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Zone) || !Marker)
	{
		return;
	}

	for (const FGP_EnemySpawnEntry& Entry : Marker->GetSpawns())
	{
		if (!*Entry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetSpawnPointNearMarker(Marker, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] Skipped marker spawn in zone '%s': no navmesh near marker. Check NavMeshBoundsVolume coverage."), *Zone->GetName());
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass, SpawnLocation, FRotator::ZeroRotator, SpawnParameters);

			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			SpawnedEnemy->SpawnDefaultController();
			RegisterZoneEnemy(SpawnedEnemy, Zone->GetCorruptionRegionId());
		}
	}
}

void AGP_GameMode::StartRegionEventForZone(AGP_EnemySpawnVolume* Zone, EGPRegionEventTrigger Trigger)
{
	if (!RegionEventDirector || !IsValid(Zone))
	{
		return;
	}

	if (Trigger == EGPRegionEventTrigger::ZoneStarted && !bStartRegionEventsOnZoneStart)
	{
		return;
	}

	if (Trigger == EGPRegionEventTrigger::ZoneCompleted && !bStartRegionEventsOnZoneCompleted)
	{
		return;
	}

	RegionEventDirector->TryStartRegionEventForZone(Zone, Trigger);
}

void AGP_GameMode::MaybeCompleteZone()
{
	// In marker mode the zone clears only once every marker has fired and nothing is left alive.
	const bool bAllMarkersDone = (MarkersTotal == 0) || (MarkersTriggered >= MarkersTotal);
	if (bAllMarkersDone && AliveZoneEnemies == 0)
	{
		CompleteCurrentZone();
	}
}

void AGP_GameMode::SpawnZoneEnemies(AGP_EnemySpawnVolume* Zone)
{
	UWorld* World = GetWorld();
	if (!IsValid(Zone) || !World)
	{
		return;
	}

	const bool bRandomize = !Zone->IsBossZone();

	for (const FGP_EnemySpawnEntry& Entry : Zone->GetSpawns())
	{
		if (!*Entry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < Entry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = Zone->GetSpawnPoint(bRandomize, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] Skipped spawn in zone '%s': no navmesh near spawn point. Check NavMeshBoundsVolume coverage."), *Zone->GetName());
				continue;
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActor<AGP_EnemyCharacter>(
				Entry.EnemyClass,
				SpawnLocation,
				FRotator::ZeroRotator,
				SpawnParameters);

			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			SpawnedEnemy->SpawnDefaultController();
			RegisterZoneEnemy(SpawnedEnemy, Zone->GetCorruptionRegionId());
		}
	}
}

void AGP_GameMode::RegisterZoneEnemy(AGP_EnemyCharacter* Enemy, int32 CorruptionRegionId)
{
	if (bRunFinished || !IsValid(Enemy))
	{
		return;
	}

	if (Enemy->OnEnemyDied.IsAlreadyBound(this, &AGP_GameMode::HandleZoneEnemyDied))
	{
		return;
	}

	// The explicit event/zone id lets enemies retain their regional strength even after progression advances.
	Enemy->SetCorruptionRegionId(CorruptionRegionId != INDEX_NONE ? CorruptionRegionId : CurrentZoneIndex);

	Enemy->OnEnemyDied.AddDynamic(this, &AGP_GameMode::HandleZoneEnemyDied);
	++AliveZoneEnemies;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}
}

void AGP_GameMode::HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy)
{
	AliveZoneEnemies = FMath::Max(0, AliveZoneEnemies - 1);

	// Remember where this enemy fell so the advance portal can spawn on the kill spot.
	if (IsValid(DeadEnemy))
	{
		LastEnemyDeathLocation = DeadEnemy->GetActorLocation();
		bHasLastDeathLocation = true;
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}

	MaybeCompleteZone();
}

void AGP_GameMode::HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy)
{
	// Event-spawned enemies join the current zone budget so events cannot be ignored during a city clear.
	RegisterZoneEnemy(Enemy, IsValid(EventActor) ? EventActor->GetRegionId() : INDEX_NONE);
}

void AGP_GameMode::CompleteCurrentZone()
{
	if (bRunFinished || !OrderedZones.IsValidIndex(CurrentZoneIndex))
	{
		return;
	}

	AGP_EnemySpawnVolume* ClearedZone = OrderedZones[CurrentZoneIndex];

	if (RegionEventDirector)
	{
		// Finish any active region event before the zone writes its final revived state.
		RegionEventDirector->CompleteEventsForZone(ClearedZone);
	}

	// Revive this zone's regions: the cleared city brings its surrounding nature
	// regions back to life. Server writes the replicated state; clients apply the
	// landscape/vegetation change via GameState's OnRegionStateChanged.
	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		for (int32 RegionId : ClearedZone->GetRegionsToRevive())
		{
			GPGameState->SetRegionState(RegionId, AliveRegionState);
		}
	}

	OnZoneCompleted(CurrentZoneIndex, ClearedZone);
	StartRegionEventForZone(ClearedZone, EGPRegionEventTrigger::ZoneCompleted);
	AdvanceZone();
}

void AGP_GameMode::AdvanceZone()
{
	const int32 NextZoneIndex = CurrentZoneIndex + 1;

	if (!OrderedZones.IsValidIndex(NextZoneIndex))
	{
		FinishRun(/*bVictory=*/true);
		return;
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetMatchPhase(EGPMatchPhase::Transition);
	}

	// Unlock next zone — spawn fires when player physically enters it.
	UnlockZone(NextZoneIndex);

	// Drop a portal at the cleared zone that teleports the party into the next zone's box.
	SpawnPortalToZone(CurrentZoneIndex, NextZoneIndex);
}

void AGP_GameMode::SpawnPortalToZone(int32 FromZoneIndex, int32 ToZoneIndex)
{
	UWorld* World = GetWorld();
	if (!World || !*PortalClass || !OrderedZones.IsValidIndex(FromZoneIndex) || !OrderedZones.IsValidIndex(ToZoneIndex))
	{
		// No portal class set (or bad index): fall back to walking into the unlocked zone.
		return;
	}

	bool bProjected = false;
	// Spawn on the spot where the last monster fell; fall back to the cleared zone
	// center if no enemy death was recorded (e.g. an empty zone auto-completing).
	const FVector SpawnPos = bHasLastDeathLocation
		? LastEnemyDeathLocation
		: OrderedZones[FromZoneIndex]->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);
	bool bTargetProjected = false;
	const FVector TargetPos = OrderedZones[ToZoneIndex]->GetSpawnPoint(/*bRandomizeInVolume=*/false, bTargetProjected);
	if (!bTargetProjected)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GP_GameMode] Skipped portal from zone %d to %d: no navmesh near target zone '%s'."),
			FromZoneIndex,
			ToZoneIndex,
			*OrderedZones[ToZoneIndex]->GetName());
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_RunPortal* Portal = World->SpawnActor<AGP_RunPortal>(PortalClass, SpawnPos, FRotator::ZeroRotator, SpawnParameters);
	if (IsValid(Portal))
	{
		Portal->SetTargetLocation(TargetPos);
	}
}

void AGP_GameMode::NotifyAllPlayersDead()
{
	FinishRun(/*bVictory=*/false);
}

void AGP_GameMode::FinishRun(bool bVictory)
{
	if (bRunFinished)
	{
		return;
	}

	bRunFinished = true;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetMatchPhase(bVictory ? EGPMatchPhase::Victory : EGPMatchPhase::Defeat);
	}

	OnRunFinished(bVictory);

	// Send the party back to the lobby after a beat so the result screen shows.
	if (UWorld* World = GetWorld())
	{
		if (ReturnToLobbyDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(ReturnToLobbyTimerHandle, this,
				&AGP_GameMode::ReturnToLobby, ReturnToLobbyDelay, false);
		}
		else
		{
			ReturnToLobby();
		}
	}
}

void AGP_GameMode::ReturnToLobby()
{
	if (UWorld* World = GetWorld())
	{
		// Seamless ServerTravel carries every connected client back together.
		const FString URL = FString::Printf(TEXT("/Game/Maps/%s?listen"), *ReturnMapName);
		World->ServerTravel(URL);
	}
}

AGP_GameState* AGP_GameMode::GetGPGameState() const
{
	return GetGameState<AGP_GameState>();
}
