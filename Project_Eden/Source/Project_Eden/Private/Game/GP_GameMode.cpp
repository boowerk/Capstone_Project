#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Game/Corruption/GP_CorruptionPresentationActor.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunSeed.h"
#include "Game/GP_RunPortal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Player/GP_PlayerController.h"
#include "UI/GP_MinimapSubsystem.h"

AGP_GameMode::AGP_GameMode()
{
	GameStateClass = AGP_GameState::StaticClass();
	RegionEventDirectorClass = AGP_RegionEventDirector::StaticClass();
	CorruptionPresentationClass = AGP_CorruptionPresentationActor::StaticClass();
	VillageLayoutDirectorClass = AGP_VillageLayoutDirector::StaticClass();

	// Match the lobby's seamless transition so arriving clients are carried
	// into this map instead of being dropped during the load.
	bUseSeamlessTravel = true;
}

void AGP_GameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!GPRunSeed::TryParse(Options, RunSeed))
	{
		RunSeed = GPRunSeed::Generate();
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] No valid RunSeed option was provided; generated fallback seed %d."),
			RunSeed);
	}
}

void AGP_GameMode::InitGameState()
{
	Super::InitGameState();

	if (AGP_GameState* GPGameState = GetGameState<AGP_GameState>())
	{
		GPGameState->SetRunSeed(RunSeed);
	}
}

void AGP_GameMode::BeginPlay()
{
	Super::BeginPlay();

	ResolveVillageLayoutDirector();
	ResolveRegionEventDirector();
	SpawnCorruptionPresentation();

	// Let placed BP actors bind to GameState delegates in their BeginPlay before
	// the initial all-dead reset is broadcast on the server/listen host.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &AGP_GameMode::InitializeRegionStates));
	}

	if (VillageLayoutDirector)
	{
		VillageLayoutDirector->OnRuntimeLayoutReady.AddUniqueDynamic(
			this,
			&AGP_GameMode::HandleVillageRuntimeLayoutReady);
		if (VillageLayoutDirector->IsRuntimeLayoutReady())
		{
			HandleVillageRuntimeLayoutReady();
		}
	}
	else
	{
		InitializeZoneProgression();
	}
}

void AGP_GameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterAllZoneNavigationInvokers();
	Super::EndPlay(EndPlayReason);
}

void AGP_GameMode::ResolveVillageLayoutDirector()
{
	VillageLayoutDirector = Cast<AGP_VillageLayoutDirector>(
		UGameplayStatics::GetActorOfClass(this, AGP_VillageLayoutDirector::StaticClass()));

	if (!VillageLayoutDirector && bAutoSpawnVillageLayoutDirector && *VillageLayoutDirectorClass)
	{
		if (UWorld* World = GetWorld())
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			VillageLayoutDirector = World->SpawnActor<AGP_VillageLayoutDirector>(
				VillageLayoutDirectorClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
		}
	}
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
		if (A.GetZoneOrder() != B.GetZoneOrder())
		{
			return A.GetZoneOrder() < B.GetZoneOrder();
		}
		return A.GetZoneId().LexicalLess(B.GetZoneId());
	});
}

void AGP_GameMode::InitializeZoneProgression()
{
	if (bZoneProgressionInitialized)
	{
		return;
	}

	GatherZones();
	bZoneProgressionInitialized = true;
	bUseStagedZoneProgression = OrderedZones.ContainsByPredicate(
		[](const AGP_EnemySpawnVolume* Zone)
		{
			return IsValid(Zone) && Zone->GetZoneStage() != EGPZoneStage::Legacy;
		});

	ZoneRuntimeStates.Reset();
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (!IsValid(Zone))
		{
			continue;
		}

		BindZoneDelegates(Zone);
		FGPZoneRuntimeState& State = ZoneRuntimeStates.FindOrAdd(Zone);
		State.Zone = Zone;
	}

	if (OrderedZones.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] No AGP_EnemySpawnVolume zones found after village layout became ready."));
		return;
	}

	if (bUseStagedZoneProgression)
	{
		UnlockStage(EGPZoneStage::Outer);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Initialized staged zone progression with %d zones."),
			OrderedZones.Num());
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &AGP_GameMode::AssignPlayersToOuterVillageStarts));
		}
		return;
	}

	UnlockZone(0);
}

void AGP_GameMode::BindZoneDelegates(AGP_EnemySpawnVolume* Zone)
{
	if (!IsValid(Zone))
	{
		return;
	}

	Zone->OnPlayerEnteredZone.AddUniqueDynamic(this, &AGP_GameMode::HandlePlayerEnteredZone);
	Zone->OnPlayerEnteredZoneDetailed.AddUniqueDynamic(
		this,
		&AGP_GameMode::HandlePlayerEnteredZoneDetailed);
	Zone->OnPlayerExitedZoneDetailed.AddUniqueDynamic(
		this,
		&AGP_GameMode::HandlePlayerExitedZoneDetailed);
	Zone->OnMarkerTriggered.AddUniqueDynamic(this, &AGP_GameMode::HandleMarkerTriggered);
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
	RegisterZoneNavigationInvoker(OrderedZones[ZoneIndex]);
	OrderedZones[ZoneIndex]->Unlock();

	UE_LOG(LogTemp, Log, TEXT("[GP_GameMode] Zone %d unlocked — waiting for player to enter."), ZoneIndex);
}

void AGP_GameMode::HandlePlayerEnteredZone(AGP_EnemySpawnVolume* Zone)
{
	if (bRunFinished || !IsValid(Zone))
	{
		return;
	}

	if (bUseStagedZoneProgression)
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

void AGP_GameMode::HandlePlayerEnteredZoneDetailed(
	AGP_EnemySpawnVolume* Zone,
	APlayerState* PlayerState)
{
	if (bRunFinished || !bUseStagedZoneProgression || !IsValid(Zone) || !IsValid(PlayerState))
	{
		return;
	}

	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (!State || State->bCompleted)
	{
		return;
	}

	State->Participants.Add(PlayerState);
	State->PresentPlayers.Add(PlayerState);

	if (!State->bStarted)
	{
		const bool bRequiresFullParty =
			Zone->GetZoneStage() == EGPZoneStage::Center
			|| Zone->GetZoneStage() == EGPZoneStage::Colosseum;
		if (!bRequiresFullParty || AreAllActivePlayersPresent(*State))
		{
			StartStagedZone(Zone);
		}
	}
}

void AGP_GameMode::RegisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone)
{
	if (!bUseZoneNavigationInvokers || !IsValid(Zone)
		|| RegisteredNavigationInvokerZones.Contains(Zone))
	{
		return;
	}

	const float GenerationRadius = FMath::Max(1000.0f, ZoneNavigationGenerationRadius);
	const float RemovalRadius = FMath::Max(GenerationRadius, ZoneNavigationRemovalRadius);
	UNavigationSystemV1::RegisterNavigationInvoker(*Zone, GenerationRadius, RemovalRadius);
	RegisteredNavigationInvokerZones.Add(Zone);

	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Registered zone '%s' as navigation invoker (generation=%.0f cm, removal=%.0f cm)."),
		*Zone->GetZoneId().ToString(),
		GenerationRadius,
		RemovalRadius);
}

void AGP_GameMode::UnregisterZoneNavigationInvoker(AGP_EnemySpawnVolume* Zone)
{
	ZoneNavigationStartRetries.Remove(Zone);
	if (!IsValid(Zone) || !RegisteredNavigationInvokerZones.Remove(Zone))
	{
		return;
	}

	UNavigationSystemV1::UnregisterNavigationInvoker(*Zone);
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Unregistered completed zone '%s' navigation invoker."),
		*Zone->GetZoneId().ToString());
}

void AGP_GameMode::UnregisterAllZoneNavigationInvokers()
{
	for (const TWeakObjectPtr<AGP_EnemySpawnVolume>& Zone : RegisteredNavigationInvokerZones)
	{
		if (Zone.IsValid())
		{
			UNavigationSystemV1::UnregisterNavigationInvoker(*Zone.Get());
		}
	}
	RegisteredNavigationInvokerZones.Reset();
	ZoneNavigationStartRetries.Reset();
}

void AGP_GameMode::HandlePlayerExitedZoneDetailed(
	AGP_EnemySpawnVolume* Zone,
	APlayerState* PlayerState)
{
	if (!bUseStagedZoneProgression || !IsValid(Zone) || !IsValid(PlayerState))
	{
		return;
	}

	if (FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone))
	{
		State->PresentPlayers.Remove(PlayerState);
	}
}

void AGP_GameMode::HandleVillageRuntimeLayoutReady()
{
	InitializeZoneProgression();

	// Village Level Instances finish loading and their City PCG graphs finish generating
	// before the director broadcasts this signal. Request the final minimap capture here
	// so the static map includes the run's selected villages rather than an earlier
	// global-PCG-only layout.
	if (UWorld* World = GetWorld())
	{
		if (UGP_MinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UGP_MinimapSubsystem>())
		{
			MinimapSubsystem->NotifyPcgLayoutReady(0.25f);
		}
	}
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

	if (bUseStagedZoneProgression)
	{
		UnlockStage(EGPZoneStage::Outer);
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
	if (bRunFinished || !IsValid(Zone) || !Marker)
	{
		return;
	}

	if (bUseStagedZoneProgression)
	{
		FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State || !State->bStarted || State->bCompleted)
		{
			return;
		}

		++State->MarkersTriggered;
		SpawnMarkerEnemies(Zone, Marker);
		MaybeCompleteStagedZone(Zone);
		return;
	}

	if (OrderedZones.IndexOfByKey(Zone) != CurrentZoneIndex)
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
			RegisterZoneEnemy(SpawnedEnemy, Zone, Zone->GetCorruptionRegionId());
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
			RegisterZoneEnemy(SpawnedEnemy, Zone, Zone->GetCorruptionRegionId());
		}
	}
}

void AGP_GameMode::RegisterZoneEnemy(
	AGP_EnemyCharacter* Enemy,
	AGP_EnemySpawnVolume* OwningZone,
	int32 CorruptionRegionId)
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
	if (bUseStagedZoneProgression && IsValid(OwningZone))
	{
		FGPZoneRuntimeState& State = ZoneRuntimeStates.FindOrAdd(OwningZone);
		State.Zone = OwningZone;
		++State.AliveEnemies;
		EnemyOwningZones.Add(Enemy, OwningZone);
	}
	else
	{
		++AliveZoneEnemies;
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(
			bUseStagedZoneProgression ? GetTotalAliveZoneEnemies() : AliveZoneEnemies);
	}
}

void AGP_GameMode::HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy)
{
	if (bUseStagedZoneProgression)
	{
		TWeakObjectPtr<AGP_EnemySpawnVolume> OwningZone;
		if (EnemyOwningZones.RemoveAndCopyValue(DeadEnemy, OwningZone))
		{
			if (FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(OwningZone))
			{
				State->AliveEnemies = FMath::Max(0, State->AliveEnemies - 1);
				if (IsValid(DeadEnemy))
				{
					State->LastEnemyDeathLocation = DeadEnemy->GetActorLocation();
					State->bHasLastEnemyDeathLocation = true;
				}
			}

			if (AGP_GameState* GPGameState = GetGPGameState())
			{
				GPGameState->SetEnemiesRemaining(GetTotalAliveZoneEnemies());
			}
			MaybeCompleteStagedZone(OwningZone.Get());
		}
		return;
	}

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
	const int32 RegionId = IsValid(EventActor) ? EventActor->GetRegionId() : INDEX_NONE;
	RegisterZoneEnemy(
		Enemy,
		bUseStagedZoneProgression ? FindActiveZoneForRegion(RegionId) : nullptr,
		RegionId);
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
	UnregisterZoneNavigationInvoker(ClearedZone);
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

void AGP_GameMode::UnlockStage(EGPZoneStage Stage)
{
	if (UnlockedStages.Contains(Stage))
	{
		return;
	}

	int32 UnlockedCount = 0;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (IsValid(Zone) && Zone->GetZoneStage() == Stage)
		{
			RegisterZoneNavigationInvoker(Zone);
			Zone->Unlock();
			++UnlockedCount;
		}
	}

	if (UnlockedCount > 0)
	{
		UnlockedStages.Add(Stage);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Stage %d unlocked with %d zone(s)."),
			static_cast<int32>(Stage),
			UnlockedCount);
	}
}

void AGP_GameMode::StartStagedZone(AGP_EnemySpawnVolume* Zone)
{
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (bRunFinished || !State || State->bStarted || State->bCompleted || !IsValid(Zone))
	{
		return;
	}

	const bool bRequiresFullParty =
		Zone->GetZoneStage() == EGPZoneStage::Center
		|| Zone->GetZoneStage() == EGPZoneStage::Colosseum;
	if (bRequiresFullParty && !AreAllActivePlayersPresent(*State))
	{
		return;
	}

	if (bUseZoneNavigationInvokers)
	{
		bool bNavigationReady = false;
		Zone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bNavigationReady);
		if (!bNavigationReady)
		{
			const float RetryInterval = FMath::Max(0.05f, ZoneNavigationRetryInterval);
			const int32 MaxRetries = FMath::Max(
				1,
				FMath::CeilToInt(ZoneNavigationReadyTimeout / RetryInterval));
			int32& RetryCount = ZoneNavigationStartRetries.FindOrAdd(Zone);
			if (RetryCount >= MaxRetries)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[GP_GameMode] Zone '%s' could not start: navigation was not ready after %.1f seconds. Check NavMeshBoundsVolume coverage and RecastNavMesh Runtime Generation=Dynamic."),
					*Zone->GetZoneId().ToString(),
					ZoneNavigationReadyTimeout);
				return;
			}

			++RetryCount;
			if (UWorld* World = GetWorld())
			{
				const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakZone = Zone;
				FTimerHandle RetryTimer;
				World->GetTimerManager().SetTimer(
					RetryTimer,
					FTimerDelegate::CreateWeakLambda(this, [this, WeakZone]()
					{
						if (WeakZone.IsValid())
						{
							StartStagedZone(WeakZone.Get());
						}
					}),
					RetryInterval,
					false);
			}
			return;
		}
	}

	ZoneNavigationStartRetries.Remove(Zone);
	State->bStarted = true;
	State->MarkersTotal = Zone->GetMarkerCount();
	State->MarkersTriggered = 0;
	State->AliveEnemies = 0;
	State->bHasLastEnemyDeathLocation = false;
	bRunStarted = true;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetCurrentZoneIndex(Zone->GetZoneOrder());
		GPGameState->SetMatchPhase(
			Zone->GetZoneStage() == EGPZoneStage::Colosseum || Zone->IsBossZone()
				? EGPMatchPhase::BossFight
				: EGPMatchPhase::InCity);
	}

	StartRegionEventForZone(Zone, EGPRegionEventTrigger::ZoneStarted);
	const int32 ZoneIndex = OrderedZones.IndexOfByKey(Zone);
	OnZoneStarted(ZoneIndex, Zone);

	if (State->MarkersTotal > 0)
	{
		Zone->ActivateMarkers();
		return;
	}

	SpawnZoneEnemies(Zone);
	MaybeCompleteStagedZone(Zone);
}

void AGP_GameMode::MaybeCompleteStagedZone(AGP_EnemySpawnVolume* Zone)
{
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (!State || !State->bStarted || State->bCompleted)
	{
		return;
	}

	const bool bAllMarkersDone =
		State->MarkersTotal == 0 || State->MarkersTriggered >= State->MarkersTotal;
	if (bAllMarkersDone && State->AliveEnemies == 0)
	{
		CompleteStagedZone(Zone);
	}
}

void AGP_GameMode::CompleteStagedZone(AGP_EnemySpawnVolume* Zone)
{
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (bRunFinished || !State || State->bCompleted || !IsValid(Zone))
	{
		return;
	}

	State->bCompleted = true;
	if (RegionEventDirector)
	{
		RegionEventDirector->CompleteEventsForZone(Zone);
	}

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		for (int32 RegionId : Zone->GetRegionsToRevive())
		{
			GPGameState->SetRegionState(RegionId, AliveRegionState);
		}
	}

	if (Zone->GetZoneStage() == EGPZoneStage::Middle)
	{
		AGP_GameState* GPGameState = GetGPGameState();
		for (const TWeakObjectPtr<APlayerState>& Participant : State->Participants)
		{
			if (Participant.IsValid() && GPGameState)
			{
				GPGameState->GrantMiddleClearCredit(Participant.Get());
			}
		}
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Middle zone '%s' completed; qualified players=%d."),
			*Zone->GetZoneId().ToString(),
			GPGameState ? GPGameState->GetMiddleClearCreditCount() : 0);
	}

	OnZoneCompleted(OrderedZones.IndexOfByKey(Zone), Zone);
	StartRegionEventForZone(Zone, EGPRegionEventTrigger::ZoneCompleted);
	UnregisterZoneNavigationInvoker(Zone);
	EvaluateStagedProgression();
}

void AGP_GameMode::EvaluateStagedProgression()
{
	if (AreAllZonesInStageCompleted(EGPZoneStage::Outer)
		&& !UnlockedStages.Contains(EGPZoneStage::Middle))
	{
		UnlockStage(EGPZoneStage::Middle);
		for (AGP_EnemySpawnVolume* OuterZone : OrderedZones)
		{
			if (!IsValid(OuterZone) || OuterZone->GetZoneStage() != EGPZoneStage::Outer)
			{
				continue;
			}

			if (AGP_EnemySpawnVolume* MiddleZone =
				FindNearestZoneInStage(OuterZone, EGPZoneStage::Middle))
			{
				const FGPZoneRuntimeState* OuterState = ZoneRuntimeStates.Find(OuterZone);
				const FVector SpawnLocation =
					OuterState && OuterState->bHasLastEnemyDeathLocation
						? OuterState->LastEnemyDeathLocation
						: OuterZone->GetActorLocation();
				SpawnMiddleSelectionPortal(OuterZone, SpawnLocation);
			}
		}
	}

	if (UnlockedStages.Contains(EGPZoneStage::Middle)
		&& HaveAllActivePlayersMiddleCredit()
		&& !UnlockedStages.Contains(EGPZoneStage::Center))
	{
		UnlockStage(EGPZoneStage::Center);
		for (AGP_EnemySpawnVolume* MiddleZone : OrderedZones)
		{
			const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(MiddleZone);
			if (!IsValid(MiddleZone)
				|| MiddleZone->GetZoneStage() != EGPZoneStage::Middle
				|| !State
				|| !State->bCompleted)
			{
				continue;
			}

			if (AGP_EnemySpawnVolume* CenterZone =
				FindNearestZoneInStage(MiddleZone, EGPZoneStage::Center))
			{
				const FVector SpawnLocation = State->bHasLastEnemyDeathLocation
					? State->LastEnemyDeathLocation
					: MiddleZone->GetActorLocation();
				SpawnPortalBetweenZones(MiddleZone, CenterZone, SpawnLocation);
			}
		}
	}

	if (AreAllZonesInStageCompleted(EGPZoneStage::Center)
		&& !UnlockedStages.Contains(EGPZoneStage::Colosseum))
	{
		UnlockStage(EGPZoneStage::Colosseum);
		for (AGP_EnemySpawnVolume* CenterZone : OrderedZones)
		{
			const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(CenterZone);
			if (!IsValid(CenterZone)
				|| CenterZone->GetZoneStage() != EGPZoneStage::Center
				|| !State
				|| !State->bCompleted)
			{
				continue;
			}

			if (AGP_EnemySpawnVolume* BossZone =
				FindNearestZoneInStage(CenterZone, EGPZoneStage::Colosseum))
			{
				const FVector SpawnLocation = State->bHasLastEnemyDeathLocation
					? State->LastEnemyDeathLocation
					: CenterZone->GetActorLocation();
				SpawnPortalBetweenZones(CenterZone, BossZone, SpawnLocation);
			}
		}
	}

	if (AreAllZonesInStageCompleted(EGPZoneStage::Colosseum))
	{
		FinishRun(/*bVictory=*/true);
	}
}

bool AGP_GameMode::AreAllZonesInStageCompleted(EGPZoneStage Stage) const
{
	bool bFoundZone = false;
	for (const AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (!IsValid(Zone) || Zone->GetZoneStage() != Stage)
		{
			continue;
		}

		bFoundZone = true;
		const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State || !State->bCompleted)
		{
			return false;
		}
	}
	return bFoundZone;
}

bool AGP_GameMode::HaveAllActivePlayersMiddleCredit() const
{
	const AGP_GameState* GPGameState = GetGPGameState();
	if (!GPGameState)
	{
		return false;
	}

	int32 ActivePlayerCount = 0;
	for (APlayerState* PlayerState : GPGameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || PlayerState->IsOnlyASpectator())
		{
			continue;
		}

		++ActivePlayerCount;
		if (!GPGameState->HasMiddleClearCredit(PlayerState))
		{
			return false;
		}
	}
	return ActivePlayerCount > 0;
}

bool AGP_GameMode::AreAllActivePlayersPresent(const FGPZoneRuntimeState& State) const
{
	const AGP_GameState* GPGameState = GetGPGameState();
	if (!GPGameState)
	{
		return false;
	}

	int32 ActivePlayerCount = 0;
	for (APlayerState* PlayerState : GPGameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || PlayerState->IsOnlyASpectator())
		{
			continue;
		}

		++ActivePlayerCount;
		if (!State.PresentPlayers.Contains(PlayerState))
		{
			return false;
		}
	}
	return ActivePlayerCount > 0;
}

int32 AGP_GameMode::GetTotalAliveZoneEnemies() const
{
	int32 Total = 0;
	for (const TPair<TWeakObjectPtr<AGP_EnemySpawnVolume>, FGPZoneRuntimeState>& Pair : ZoneRuntimeStates)
	{
		if (!Pair.Value.bCompleted)
		{
			Total += Pair.Value.AliveEnemies;
		}
	}
	return Total;
}

AGP_EnemySpawnVolume* AGP_GameMode::FindActiveZoneForRegion(int32 RegionId) const
{
	for (const TPair<TWeakObjectPtr<AGP_EnemySpawnVolume>, FGPZoneRuntimeState>& Pair : ZoneRuntimeStates)
	{
		AGP_EnemySpawnVolume* Zone = Pair.Key.Get();
		if (IsValid(Zone)
			&& Pair.Value.bStarted
			&& !Pair.Value.bCompleted
			&& Zone->GetCorruptionRegionId() == RegionId)
		{
			return Zone;
		}
	}
	return nullptr;
}

AGP_EnemySpawnVolume* AGP_GameMode::FindNearestZoneInStage(
	const AGP_EnemySpawnVolume* FromZone,
	EGPZoneStage Stage) const
{
	AGP_EnemySpawnVolume* Nearest = nullptr;
	double BestDistanceSquared = TNumericLimits<double>::Max();
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (!IsValid(Zone) || Zone->GetZoneStage() != Stage)
		{
			continue;
		}

		const double DistanceSquared = FromZone
			? FVector::DistSquared(FromZone->GetActorLocation(), Zone->GetActorLocation())
			: 0.0;
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			Nearest = Zone;
		}
	}
	return Nearest;
}

void AGP_GameMode::OpenMiddleTravelSelection(
	AGP_PlayerController* PlayerController)
{
	if (!HasAuthority() || bRunFinished || !IsValid(PlayerController)
		|| !UnlockedStages.Contains(EGPZoneStage::Middle))
	{
		return;
	}

	TArray<FGPMiddleTravelDestination> Destinations;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!IsValid(Zone)
			|| Zone->GetZoneStage() != EGPZoneStage::Middle
			|| (State && State->bCompleted))
		{
			continue;
		}

		FGPMiddleTravelDestination& Destination = Destinations.AddDefaulted_GetRef();
		Destination.ZoneId = Zone->GetZoneId();
		Destination.ZoneOrder = Zone->GetZoneOrder();
		Destination.WorldLocation = Zone->GetActorLocation();
	}

	if (Destinations.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] Middle travel map was not opened: no uncleared Middle destinations remain."));
		return;
	}

	PlayerController->ClientOpenMiddleTravelMap(Destinations);
}

bool AGP_GameMode::RequestMiddleTravel(
	AGP_PlayerController* PlayerController,
	FName DestinationZoneId)
{
	if (!HasAuthority() || bRunFinished || !IsValid(PlayerController)
		|| DestinationZoneId.IsNone()
		|| !UnlockedStages.Contains(EGPZoneStage::Middle)
		|| !IsPlayerNearMiddleSelectionPortal(PlayerController))
	{
		return false;
	}

	AGP_EnemySpawnVolume* DestinationZone = nullptr;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (IsValid(Zone)
			&& Zone->GetZoneStage() == EGPZoneStage::Middle
			&& Zone->GetZoneId() == DestinationZoneId
			&& (!State || !State->bCompleted))
		{
			DestinationZone = Zone;
			break;
		}
	}

	APawn* PlayerPawn = PlayerController->GetPawn();
	if (!IsValid(DestinationZone) || !IsValid(PlayerPawn))
	{
		return false;
	}

	bool bProjected = false;
	FVector ArrivalLocation = FVector::ZeroVector;
	if (AActor* ArrivalAnchor =
		FindTaggedActorInZoneLevel(DestinationZone, MiddleArrivalAnchorTag))
	{
		ArrivalLocation = ArrivalAnchor->GetActorLocation();
		if (const UNavigationSystemV1* NavigationSystem =
			UNavigationSystemV1::GetCurrent(GetWorld()))
		{
			FNavLocation ProjectedLocation;
			bProjected = NavigationSystem->ProjectPointToNavigation(
				ArrivalLocation,
				ProjectedLocation,
				FVector(300.0f, 300.0f, 1500.0f));
			if (bProjected)
			{
				ArrivalLocation = ProjectedLocation.Location;
			}
		}
	}

	if (!bProjected)
	{
		ArrivalLocation =
			DestinationZone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);
	}

	if (!bProjected)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] Rejected Middle travel to '%s': no NavMesh at its ArrivalAnchor or Zone."),
			*DestinationZoneId.ToString());
		return false;
	}

	const float HalfHeight = PlayerPawn->GetSimpleCollisionHalfHeight();
	const FVector SafeArrival =
		ArrivalLocation + FVector(0.0f, 0.0f, HalfHeight + 10.0f);
	if (!PlayerPawn->TeleportTo(
		SafeArrival,
		PlayerPawn->GetActorRotation(),
		false,
		true))
	{
		return false;
	}

	PlayerController->ClientConfirmMiddleTravel();
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Player '%s' traveled to Middle zone '%s'."),
		*GetNameSafe(PlayerController->GetPlayerState<APlayerState>()),
		*DestinationZoneId.ToString());
	return true;
}

void AGP_GameMode::SpawnMiddleSelectionPortal(
	AGP_EnemySpawnVolume* FromZone,
	const FVector& PreferredSpawnLocation)
{
	UWorld* World = GetWorld();
	if (!World || !*PortalClass || !IsValid(FromZone))
	{
		return;
	}

	const AActor* PortalAnchor =
		FindTaggedActorInZoneLevel(FromZone, VillagePortalAnchorTag);
	const FVector SpawnLocation =
		PortalAnchor ? PortalAnchor->GetActorLocation() : PreferredSpawnLocation;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AGP_RunPortal* Portal = World->SpawnActor<AGP_RunPortal>(
		PortalClass,
		SpawnLocation,
		PortalAnchor ? PortalAnchor->GetActorRotation() : FRotator::ZeroRotator,
		SpawnParameters))
	{
		Portal->SetMiddleDestinationSelection(true);
	}
}

AActor* AGP_GameMode::FindTaggedActorInZoneLevel(
	const AGP_EnemySpawnVolume* Zone,
	FName ActorTag) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Zone) || ActorTag.IsNone())
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor)
			&& Actor != Zone
			&& Actor->GetLevel() == Zone->GetLevel()
			&& Actor->ActorHasTag(ActorTag))
		{
			return Actor;
		}
	}
	return nullptr;
}

bool AGP_GameMode::IsPlayerNearMiddleSelectionPortal(
	const AGP_PlayerController* PlayerController) const
{
	const APawn* PlayerPawn = IsValid(PlayerController)
		? PlayerController->GetPawn()
		: nullptr;
	UWorld* World = GetWorld();
	if (!World || !IsValid(PlayerPawn))
	{
		return false;
	}

	const float UseRadiusSquared = FMath::Square(
		FMath::Max(100.0f, MiddleTravelPortalUseRadius));
	for (TActorIterator<AGP_RunPortal> It(World); It; ++It)
	{
		const AGP_RunPortal* Portal = *It;
		if (IsValid(Portal)
			&& Portal->IsMiddleDestinationSelection()
			&& FVector::DistSquared(
				Portal->GetActorLocation(),
				PlayerPawn->GetActorLocation()) <= UseRadiusSquared)
		{
			return true;
		}
	}
	return false;
}

void AGP_GameMode::SpawnPortalBetweenZones(
	AGP_EnemySpawnVolume* FromZone,
	AGP_EnemySpawnVolume* ToZone,
	const FVector& PreferredSpawnLocation,
	int32 RetryCount)
{
	UWorld* World = GetWorld();
	if (!World || !*PortalClass || !IsValid(FromZone) || !IsValid(ToZone))
	{
		return;
	}

	bool bTargetProjected = false;
	const FVector TargetPosition =
		ToZone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bTargetProjected);
	if (!bTargetProjected)
	{
		const float RetryInterval = FMath::Max(0.05f, ZoneNavigationRetryInterval);
		const int32 MaxRetries = FMath::Max(
			1,
			FMath::CeilToInt(ZoneNavigationReadyTimeout / RetryInterval));
		if (bUseZoneNavigationInvokers && RetryCount < MaxRetries)
		{
			const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakFromZone = FromZone;
			const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakToZone = ToZone;
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(
				RetryTimer,
				FTimerDelegate::CreateWeakLambda(
					this,
					[this, WeakFromZone, WeakToZone, PreferredSpawnLocation, RetryCount]()
					{
						if (WeakFromZone.IsValid() && WeakToZone.IsValid())
						{
							SpawnPortalBetweenZones(
								WeakFromZone.Get(),
								WeakToZone.Get(),
								PreferredSpawnLocation,
								RetryCount + 1);
						}
					}),
				RetryInterval,
				false);
			return;
		}

		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Skipped staged portal to zone '%s': navigation was not ready after %.1f seconds."),
			*ToZone->GetZoneId().ToString(),
			ZoneNavigationReadyTimeout);
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (AGP_RunPortal* Portal = World->SpawnActor<AGP_RunPortal>(
		PortalClass,
		PreferredSpawnLocation,
		FRotator::ZeroRotator,
		SpawnParameters))
	{
		Portal->SetTargetLocation(TargetPosition);
	}
}

void AGP_GameMode::AssignPlayersToOuterVillageStarts()
{
	if (!bUseStagedZoneProgression || !GetWorld())
	{
		return;
	}

	TArray<AGP_EnemySpawnVolume*> OuterZones;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (IsValid(Zone) && Zone->GetZoneStage() == EGPZoneStage::Outer)
		{
			OuterZones.Add(Zone);
		}
	}
	OuterZones.Sort([](const AGP_EnemySpawnVolume& A, const AGP_EnemySpawnVolume& B)
	{
		return A.GetZoneId().LexicalLess(B.GetZoneId());
	});

	TArray<AActor*> FoundStarts;
	UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), FoundStarts);
	TArray<APlayerStart*> OuterStarts;
	for (AGP_EnemySpawnVolume* Zone : OuterZones)
	{
		APlayerStart* MatchingStart = nullptr;
		for (AActor* Actor : FoundStarts)
		{
			APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
			if (IsValid(PlayerStart) && PlayerStart->GetLevel() == Zone->GetLevel())
			{
				if (!MatchingStart
					|| PlayerStart->GetFName().LexicalLess(MatchingStart->GetFName()))
				{
					MatchingStart = PlayerStart;
				}
			}
		}
		if (MatchingStart)
		{
			OuterStarts.Add(MatchingStart);
		}
	}

	TArray<APlayerController*> Players;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PlayerController = It->Get())
		{
			Players.Add(PlayerController);
		}
	}
	Players.Sort([](const APlayerController& A, const APlayerController& B)
	{
		const APlayerState* AState = A.GetPlayerState<APlayerState>();
		const APlayerState* BState = B.GetPlayerState<APlayerState>();
		return (AState ? AState->GetPlayerId() : MAX_int32)
			< (BState ? BState->GetPlayerId() : MAX_int32);
	});

	const int32 AssignmentCount = FMath::Min(Players.Num(), OuterStarts.Num());
	for (int32 Index = 0; Index < AssignmentCount; ++Index)
	{
		APawn* Pawn = Players[Index] ? Players[Index]->GetPawn() : nullptr;
		APlayerStart* PlayerStart = OuterStarts[Index];
		if (!IsValid(Pawn) || !IsValid(PlayerStart))
		{
			continue;
		}

		Pawn->TeleportTo(
			PlayerStart->GetActorLocation(),
			PlayerStart->GetActorRotation(),
			false,
			true);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Assigned player %d to outer start '%s'."),
			Index,
			*PlayerStart->GetName());
	}

	if (AssignmentCount < Players.Num())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] Found %d player(s) but only %d streamed outer PlayerStart(s)."),
			Players.Num(),
			OuterStarts.Num());
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
	UnregisterAllZoneNavigationInvokers();

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
