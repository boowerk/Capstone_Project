#include "Game/GP_GameMode.h"

#include "Actors/GP_LevelBuildAnimator.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunSeed.h"
#include "Game/GP_RunPortal.h"
#include "Game/GP_RunProgressionPolicy.h"
#include "Game/GP_ThreePlayerGameSession.h"
#include "Game/Regions/GP_RegionSpatialSubsystem.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"
#include "TimerManager.h"
#include "UI/GP_MinimapSubsystem.h"

AGP_GameMode::AGP_GameMode()
{
	GameStateClass = AGP_GameState::StaticClass();
	VillageLayoutDirectorClass = AGP_VillageLayoutDirector::StaticClass();
	// Preserve the three-player admission cap if a client attempts to join
	// directly after seamless travel reaches the gameplay map.
	GameSessionClass = AGP_ThreePlayerGameSession::StaticClass();

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

AActor* AGP_GameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	EnsurePartyPlayerStarts(Player);

	const int32 SlotIndex = ResolvePartyStartSlot(Player);
	if (PartyPlayerStartSlots.IsValidIndex(SlotIndex))
	{
		if (APlayerStart* AssignedStart = PartyPlayerStartSlots[SlotIndex].Get())
		{
			return AssignedStart;
		}
	}

	// Preserve the engine fallback for maps without an authored anchor or for unexpected extra players.
	return Super::ChoosePlayerStart_Implementation(Player);
}

void AGP_GameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	bool bRelocatedToColosseum = false;
	if (AGP_PlayerCharacter* PlayerCharacter = NewPlayer ? Cast<AGP_PlayerCharacter>(NewPlayer->GetPawn()) : nullptr)
	{
		if (AGP_PlayerState* GPPlayerState = NewPlayer->GetPlayerState<AGP_PlayerState>())
		{
			// PlayerState survives seamless travel. Clear the previous run only
			// after Super produced a valid gameplay pawn; otherwise a failed
			// restart would create a pawn-less phantom survivor.
			GPPlayerState->SetEliminated(false);
		}
		PlayerCharacter->SetPartyVisualSlot(ResolvePartyStartSlot(NewPlayer));
		bRelocatedToColosseum =
			TryRelocatePlayerToActiveColosseum(NewPlayer);
	}

	// A remote client can acknowledge its village visuals before its gameplay pawn
	// exists. Retry the outer assignment after the pawn has been restarted.
	if (!bRelocatedToColosseum)
	{
		AssignPlayersToOuterVillageStarts();
	}
}

void AGP_GameMode::Logout(AController* Exiting)
{
	const TWeakObjectPtr<AController> ExitingControllerKey(Exiting);
	if (FTimerHandle* RetryTimer =
		ColosseumRelocationRetryTimers.Find(ExitingControllerKey))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*RetryTimer);
		}
	}
	ColosseumRelocationRetryTimers.Remove(ExitingControllerKey);
	ColosseumRelocationRetryCounts.Remove(ExitingControllerKey);

	// Release the deterministic slot immediately so a reconnect does not wait for garbage collection.
	PartyStartSlotByController.Remove(TWeakObjectPtr<AController>(Exiting));
	APlayerState* ExitingPlayerState =
		IsValid(Exiting) ? Exiting->GetPlayerState<APlayerState>() : nullptr;
	if (IsValid(ExitingPlayerState))
	{
		AssignedOuterZonesByPlayer.Remove(
			TWeakObjectPtr<APlayerState>(ExitingPlayerState));
	}
	if (APlayerController* PlayerController = Cast<APlayerController>(Exiting))
	{
		const TWeakObjectPtr<APlayerController> PlayerKey(PlayerController);
		VillageVisualReadyRevisions.Remove(PlayerKey);
		OuterAssignmentRevisions.Remove(PlayerKey);
	}
	Super::Logout(Exiting);

	// Controller::Destroyed removes PlayerState from GameState only after
	// Logout returns. Re-evaluate party gates on the next tick so the departing
	// member cannot keep Center/Colosseum or defeat evaluation blocked.
	if (bZoneProgressionInitialized && !bRunFinished)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					if (!bZoneProgressionInitialized || bRunFinished)
					{
						return;
					}

					TryStartPendingCenterZones();
					TryStartPendingColosseumIntros();
					EvaluateStagedProgression();
					EvaluatePartyDefeat();
				}));
		}
	}
}

void AGP_GameMode::BeginPlay()
{
	Super::BeginPlay();

	ResolveVillageLayoutDirector();
#if !UE_BUILD_SHIPPING
	BeginThreePlayerGameplaySmokeProbe();
#endif

	// Resolve map-authored seeds before the ordinary zone progression initializes region state.
	ResolveRuntimeRegionConfiguration();
	GatherZones();

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
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearAllTimersForObject(this);
	}
	ColosseumRelocationRetryTimers.Reset();
	ColosseumRelocationRetryCounts.Reset();

	for (const TPair<
		TWeakObjectPtr<AGP_LevelBuildAnimator>,
		TWeakObjectPtr<AGP_EnemySpawnVolume>>& Pair : ColosseumIntroZonesByAnimator)
	{
		if (Pair.Key.IsValid())
		{
			Pair.Key->OnBuildFinishedNative.RemoveAll(this);
		}
	}
	ColosseumIntroZonesByAnimator.Reset();
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
		if (RuntimeInitialRegionStates.Num() == RegionCount)
		{
			// Landscape maps keep their serialized biome mosaic instead of being flattened to DeadRegionState.
			GPGameState->InitRegionStatesFromArray(RuntimeInitialRegionStates);
		}
		else
		{
			// Maps without RegionSeed actors retain the original linear-run initialization contract.
			GPGameState->InitRegionStates(RegionCount, DeadRegionState);
		}

	}
}

void AGP_GameMode::ResolveRuntimeRegionConfiguration()
{
	RuntimeInitialRegionStates.Reset();

	UWorld* World = GetWorld();
	UGP_RegionSpatialSubsystem* RegionSpatial = World
		? World->GetSubsystem<UGP_RegionSpatialSubsystem>()
		: nullptr;
	if (!IsValid(RegionSpatial))
	{
		return;
	}

	const int32 AuthoredRegionCount = RegionSpatial->GetRegionCount();
	if (AuthoredRegionCount <= 0 || RegionSpatial->GetSeedCount() <= 0)
	{
		return;
	}

	// The highest placed SeedIndex defines the addressable runtime array before any dependent system initializes.
	RegionCount = AuthoredRegionCount;
	if (!RegionSpatial->BuildAuthoredRegionStates(DeadRegionState, RuntimeInitialRegionStates))
	{
		// Unsupported legacy seed schemas still get the correct count and the established uniform fallback state.
		RuntimeInitialRegionStates.Reset();
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("[GP_GameMode] Resolved %d runtime regions from %d placed seeds (authored states=%s)."),
		RegionCount,
		RegionSpatial->GetSeedCount(),
		RuntimeInitialRegionStates.Num() == RegionCount ? TEXT("preserved") : TEXT("fallback"));
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
	AssignedOuterZonesByPlayer.Reset();
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

	const bool bHasMatchingPortalArrival =
		State->PortalArrivals.Contains(PlayerState);
	if (!GPRunProgressionPolicy::CanCountStagedZonePresence(
		Zone->GetZoneStage(),
		bHasMatchingPortalArrival))
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("[GP_GameMode] Ignored non-portal Colosseum entry for player '%s' in zone '%s'."),
			*GetNameSafe(PlayerState),
			*Zone->GetZoneId().ToString());
		return;
	}

	State->Participants.Add(PlayerState);
	State->PresentPlayers.Add(PlayerState);

	if (!State->bStarted)
	{
		if (Zone->GetZoneStage() == EGPZoneStage::Colosseum)
		{
			TryStartColosseumIntro(Zone);
		}
		else
		{
			const bool bRequiresFullParty =
				Zone->GetZoneStage() == EGPZoneStage::Center;
			if (!bRequiresFullParty || AreAllActivePlayersPresent(*State))
			{
				StartStagedZone(Zone);
			}
		}
	}
}

void AGP_GameMode::NotifyPlayerArrivedViaPortal(
	AGP_PlayerCharacter* PlayerCharacter,
	AGP_EnemySpawnVolume* DestinationZone)
{
	if (!HasAuthority()
		|| bRunFinished
		|| !bUseStagedZoneProgression
		|| !IsValid(PlayerCharacter)
		|| !IsValid(DestinationZone)
		|| DestinationZone->GetZoneStage() != EGPZoneStage::Colosseum
		|| !UnlockedStages.Contains(EGPZoneStage::Colosseum))
	{
		return;
	}

	APlayerState* PlayerState = PlayerCharacter->GetPlayerState();
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(DestinationZone);
	if (!IsValid(PlayerState) || !State || State->bCompleted)
	{
		return;
	}

	State->PortalArrivals.Add(PlayerState);

	// TeleportTo can update overlaps before it returns. Record the authoritative
	// credit and process entry explicitly so callback ordering cannot lose arrival.
	HandlePlayerEnteredZoneDetailed(DestinationZone, PlayerState);

	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Credited portal arrival for player '%s' in Colosseum zone '%s'."),
		*GetNameSafe(PlayerState),
		*DestinationZone->GetZoneId().ToString());
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
	if (FTimerHandle* RetryTimer = ZoneNavigationStartRetryTimers.Find(Zone))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*RetryTimer);
		}
		ZoneNavigationStartRetryTimers.Remove(Zone);
	}
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
	if (UWorld* World = GetWorld())
	{
		for (TPair<TWeakObjectPtr<AGP_EnemySpawnVolume>, FTimerHandle>& Pair :
			ZoneNavigationStartRetryTimers)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	ZoneNavigationStartRetryTimers.Reset();

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

void AGP_GameMode::NotifyVillageClientVisualReady(
	AGP_PlayerController* PlayerController,
	int32 LayoutRevision)
{
	if (!HasAuthority() || !IsValid(PlayerController) || !IsValid(VillageLayoutDirector))
	{
		return;
	}

	const int32 CurrentRevision = VillageLayoutDirector->GetRuntimeLayoutRevision();
	if (LayoutRevision <= 0 || LayoutRevision != CurrentRevision)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[GP_GameMode] Ignored village visual-ready ACK from '%s': ClientRevision=%d ServerRevision=%d."),
			*PlayerController->GetName(),
			LayoutRevision,
			CurrentRevision);
		return;
	}

	VillageVisualReadyRevisions.Add(
		TWeakObjectPtr<APlayerController>(PlayerController),
		LayoutRevision);
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Village visuals ready for '%s' at Revision=%d."),
		*PlayerController->GetName(),
		LayoutRevision);

	if (bZoneProgressionInitialized)
	{
		AssignPlayersToOuterVillageStarts();
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
	PendingZoneEnemySpawns = 0;
	CurrentZoneBossSpawnRetryCount = 0;
	bCurrentZoneBossSpawnRetryPending = false;
	bCurrentZoneBossPhaseStarted = false;
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
		OnZoneStarted(ZoneIndex, Zone);
		return;
	}

	// Box-fallback mode: spawn the whole composition at once.
	SpawnZoneEnemies(Zone);
	OnZoneStarted(ZoneIndex, Zone);

	MaybeCompleteZone();
}

void AGP_GameMode::CompleteCurrentZone()
{
	if (bRunFinished || !OrderedZones.IsValidIndex(CurrentZoneIndex))
	{
		return;
	}

	AGP_EnemySpawnVolume* ClearedZone = OrderedZones[CurrentZoneIndex];

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
	if (!World || !OrderedZones.IsValidIndex(FromZoneIndex) || !OrderedZones.IsValidIndex(ToZoneIndex))
	{
		return;
	}
	if (!*PortalClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Cannot spawn portal from zone %d to %d: PortalClass is not configured or was not cooked."),
			FromZoneIndex,
			ToZoneIndex);
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

	const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnPos);
	AGP_RunPortal* Portal = World->SpawnActorDeferred<AGP_RunPortal>(
		PortalClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	if (IsValid(Portal))
	{
		Portal->SetDestination(OrderedZones[ToZoneIndex], TargetPos);
		UGameplayStatics::FinishSpawningActor(Portal, SpawnTransform);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Spawned portal '%s' from zone %d to %d."),
			*Portal->GetName(),
			FromZoneIndex,
			ToZoneIndex);
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] SpawnActor failed for portal from zone %d to %d using class '%s'."),
			FromZoneIndex,
			ToZoneIndex,
			*GetNameSafe(PortalClass.Get()));
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

	if (Zone->GetZoneStage() == EGPZoneStage::Colosseum
		&& !State->bIntroCompleted)
	{
		TryStartColosseumIntro(Zone);
		return;
	}

	// Colosseum already froze the admission decision when every active player
	// was present and the intro began. Rechecking PlayerArray after the intro
	// would let a join/reconnect during playback permanently block the boss.
	const bool bRequiresFullParty =
		GPRunProgressionPolicy::RequiresFullPartyAtEncounterStart(
			Zone->GetZoneStage());
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

			// Overlap, portal-arrival, and intro completion paths can all ask to
			// start the same zone while navigation is dirty. Keep exactly one
			// retry chain per zone instead of multiplying 4 Hz polling timers.
			if (const FTimerHandle* ExistingTimer =
					ZoneNavigationStartRetryTimers.Find(Zone);
				ExistingTimer && GetWorld()
				&& GetWorld()->GetTimerManager().IsTimerActive(*ExistingTimer))
			{
				return;
			}

			int32& RetryCount = ZoneNavigationStartRetries.FindOrAdd(Zone);
			if (RetryCount == MaxRetries)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[GP_GameMode] Zone '%s' navigation is still not ready after %.1f seconds; retries will continue. Check NavMeshBoundsVolume coverage and RecastNavMesh Runtime Generation=Dynamic."),
					*Zone->GetZoneId().ToString(),
					ZoneNavigationReadyTimeout);
			}

			// Navigation can remain dirty while the Colosseum pieces finish
			// moving. A fixed retry cutoff would permanently soft-lock the
			// encounter even if Recast finishes a moment later.
			RetryCount = FMath::Min(RetryCount + 1, MaxRetries + 1);
			if (UWorld* World = GetWorld())
			{
				const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakZone = Zone;
				FTimerHandle& RetryTimer =
					ZoneNavigationStartRetryTimers.FindOrAdd(WeakZone);
				World->GetTimerManager().SetTimer(
					RetryTimer,
					FTimerDelegate::CreateWeakLambda(this, [this, WeakZone]()
					{
						ZoneNavigationStartRetryTimers.Remove(WeakZone);
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

	if (FTimerHandle* RetryTimer = ZoneNavigationStartRetryTimers.Find(Zone))
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(*RetryTimer);
		}
		ZoneNavigationStartRetryTimers.Remove(Zone);
	}
	ZoneNavigationStartRetries.Remove(Zone);
	State->bStarted = true;
	State->MarkersTotal = Zone->GetMarkerCount();
	State->MarkersTriggered = 0;
	State->AliveEnemies = 0;
	State->PendingEnemySpawns = 0;
	State->BossSpawnRetryCount = 0;
	State->bBossSpawnRetryPending = false;
	State->bBossPhaseStarted = false;
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

void AGP_GameMode::TryStartColosseumIntro(AGP_EnemySpawnVolume* Zone)
{
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (bRunFinished
		|| !IsValid(Zone)
		|| Zone->GetZoneStage() != EGPZoneStage::Colosseum
		|| !State
		|| State->bStarted
		|| State->bCompleted)
	{
		return;
	}

	if (State->bIntroCompleted)
	{
		StartStagedZone(Zone);
		return;
	}

	if (!GPRunProgressionPolicy::ShouldStartColosseumIntro(
		AreAllActivePlayersPresent(*State),
		State->bIntroStarted,
		State->bIntroCompleted))
	{
		return;
	}

	State->bIntroStarted = true;
	AGP_LevelBuildAnimator* Animator = FindColosseumBuildAnimator(Zone);
	if (!IsValid(Animator))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Colosseum zone '%s' has no GP_LevelBuildAnimator in its level; skipping the entrance presentation to avoid a run softlock."),
			*Zone->GetZoneId().ToString());
		State->bIntroCompleted = true;
		StartStagedZone(Zone);
		return;
	}

	ColosseumIntroZonesByAnimator.Add(Animator, Zone);
	Animator->OnBuildFinishedNative.RemoveAll(this);
	Animator->OnBuildFinishedNative.AddUObject(
		this,
		&ThisClass::HandleColosseumBuildFinished);

	if (Animator->GetPlaybackPhase() == EGPLevelBuildPlaybackPhase::Finished)
	{
		HandleColosseumBuildFinished(Animator);
		return;
	}

	if (Animator->GetPlaybackPhase() == EGPLevelBuildPlaybackPhase::Playing)
	{
		return;
	}

	if (!Animator->StartBuildAuthoritative())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Colosseum animator '%s' failed to start for zone '%s'; skipping the entrance presentation to avoid a run softlock."),
			*GetNameSafe(Animator),
			*Zone->GetZoneId().ToString());
		HandleColosseumBuildFinished(Animator);
		return;
	}

	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Started Colosseum entrance presentation '%s' after the active party arrived through the portal."),
		*GetNameSafe(Animator));
}

void AGP_GameMode::TryStartPendingColosseumIntros()
{
	for (TPair<
		TWeakObjectPtr<AGP_EnemySpawnVolume>,
		FGPZoneRuntimeState>& Pair : ZoneRuntimeStates)
	{
		AGP_EnemySpawnVolume* Zone = Pair.Key.Get();
		if (IsValid(Zone)
			&& Zone->GetZoneStage() == EGPZoneStage::Colosseum
			&& !Pair.Value.bStarted
			&& !Pair.Value.bCompleted)
		{
			TryStartColosseumIntro(Zone);
		}
	}
}

void AGP_GameMode::TryStartPendingCenterZones()
{
	for (TPair<
		TWeakObjectPtr<AGP_EnemySpawnVolume>,
		FGPZoneRuntimeState>& Pair : ZoneRuntimeStates)
	{
		AGP_EnemySpawnVolume* Zone = Pair.Key.Get();
		if (IsValid(Zone)
			&& Zone->GetZoneStage() == EGPZoneStage::Center
			&& !Pair.Value.bStarted
			&& !Pair.Value.bCompleted)
		{
			// StartStagedZone performs the authoritative active-party presence
			// check again after the departing PlayerState leaves PlayerArray.
			StartStagedZone(Zone);
		}
	}
}

AGP_LevelBuildAnimator* AGP_GameMode::FindColosseumBuildAnimator(
	const AGP_EnemySpawnVolume* Zone) const
{
	UWorld* World = GetWorld();
	if (!World || !IsValid(Zone))
	{
		return nullptr;
	}

	AGP_LevelBuildAnimator* NearestExactAnimator = nullptr;
	AGP_LevelBuildAnimator* NearestFallbackAnimator = nullptr;
	double NearestExactDistanceSquared = TNumericLimits<double>::Max();
	double NearestFallbackDistanceSquared = TNumericLimits<double>::Max();
	for (TActorIterator<AGP_LevelBuildAnimator> It(World); It; ++It)
	{
		AGP_LevelBuildAnimator* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}

		const TWeakObjectPtr<AGP_EnemySpawnVolume>* ExistingZone =
			ColosseumIntroZonesByAnimator.Find(Candidate);
		if (ExistingZone
			&& ExistingZone->IsValid()
			&& ExistingZone->Get() != Zone)
		{
			continue;
		}

		const FName ConfiguredZoneId = Candidate->GetColosseumZoneId();
		const bool bExactZoneMatch =
			!ConfiguredZoneId.IsNone()
			&& ConfiguredZoneId == Zone->GetZoneId();
		if (!ConfiguredZoneId.IsNone() && !bExactZoneMatch)
		{
			continue;
		}

		const double DistanceSquared = FVector::DistSquared(
			Candidate->GetActorLocation(),
			Zone->GetActorLocation());
		if (bExactZoneMatch
			&& DistanceSquared < NearestExactDistanceSquared)
		{
			NearestExactDistanceSquared = DistanceSquared;
			NearestExactAnimator = Candidate;
		}
		else if (ConfiguredZoneId.IsNone()
			&& DistanceSquared < NearestFallbackDistanceSquared)
		{
			NearestFallbackDistanceSquared = DistanceSquared;
			NearestFallbackAnimator = Candidate;
		}
	}
	return NearestExactAnimator
		? NearestExactAnimator
		: NearestFallbackAnimator;
}

void AGP_GameMode::HandleColosseumBuildFinished(
	AGP_LevelBuildAnimator* Animator)
{
	if (!HasAuthority() || bRunFinished || !IsValid(Animator))
	{
		return;
	}

	const TWeakObjectPtr<AGP_EnemySpawnVolume>* ZonePtr =
		ColosseumIntroZonesByAnimator.Find(Animator);
	AGP_EnemySpawnVolume* Zone = ZonePtr ? ZonePtr->Get() : nullptr;
	if (!IsValid(Zone))
	{
		return;
	}

	Animator->OnBuildFinishedNative.RemoveAll(this);
	ColosseumIntroZonesByAnimator.Remove(Animator);

	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (!State || State->bCompleted)
	{
		return;
	}

	State->bIntroStarted = true;
	State->bIntroCompleted = true;
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Colosseum entrance presentation finished for zone '%s'; starting the boss encounter."),
		*Zone->GetZoneId().ToString());
	StartStagedZone(Zone);
}

void AGP_GameMode::MaybeCompleteStagedZone(AGP_EnemySpawnVolume* Zone)
{
	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (bRunFinished || !State || !State->bStarted || State->bCompleted)
	{
		return;
	}

	const bool bAllMarkersDone =
		State->MarkersTotal == 0 || State->MarkersTriggered >= State->MarkersTotal;
	if (bAllMarkersDone
		&& State->AliveEnemies == 0
		&& State->PendingEnemySpawns == 0)
	{
		if (Zone->HasBossPhase() && !State->bBossPhaseStarted)
		{
			if (State->bBossSpawnRetryPending)
			{
				return;
			}

			State->bBossPhaseStarted = true;
			const int32 BossSpawnCount = SpawnZoneBossEnemies(Zone);
			if (BossSpawnCount > 0)
			{
				State->BossSpawnRetryCount = 0;
				return;
			}
			if (BossSpawnCount == INDEX_NONE)
			{
				return;
			}

			// A missing nav tile is not a defeated boss. Keep the encounter
			// blocked and retry instead of incorrectly opening the next portal.
			State->bBossPhaseStarted = false;
			ScheduleZoneBossSpawnRetry(Zone);
			return;
		}
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
	UnregisterZoneNavigationInvoker(Zone);
	EvaluateStagedProgression();
}

void AGP_GameMode::EvaluateStagedProgression()
{
	if (bRunFinished)
	{
		return;
	}

	TArray<AGP_EnemySpawnVolume*> ActiveOuterZones;
	if (GetActiveAssignedOuterZones(ActiveOuterZones)
		&& !UnlockedStages.Contains(EGPZoneStage::Middle))
	{
		UnlockStage(EGPZoneStage::Middle);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] All %d active-player Outer zone(s) completed; advancing without requiring unused authored Outer slots."),
			ActiveOuterZones.Num());
		for (AGP_EnemySpawnVolume* OuterZone : ActiveOuterZones)
		{
			if (!IsValid(OuterZone))
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

bool AGP_GameMode::GetActiveAssignedOuterZones(
	TArray<AGP_EnemySpawnVolume*>& OutAssignedZones) const
{
	OutAssignedZones.Reset();

	const AGP_GameState* GPGameState = GetGPGameState();
	if (!GPGameState)
	{
		return false;
	}

	int32 ActivePlayerCount = 0;
	TSet<TWeakObjectPtr<AGP_EnemySpawnVolume>> UniqueAssignedZones;
	for (APlayerState* PlayerState : GPGameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || PlayerState->IsOnlyASpectator())
		{
			continue;
		}

		++ActivePlayerCount;
		const TWeakObjectPtr<AGP_EnemySpawnVolume>* AssignedZone =
			AssignedOuterZonesByPlayer.Find(
				TWeakObjectPtr<APlayerState>(PlayerState));
		if (AssignedZone
			&& AssignedZone->IsValid()
			&& AssignedZone->Get()->GetZoneStage() == EGPZoneStage::Outer
			&& ZoneRuntimeStates.Contains(*AssignedZone))
		{
			UniqueAssignedZones.Add(*AssignedZone);
		}
	}

	int32 CompletedAssignedZoneCount = 0;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (!IsValid(Zone)
			|| !UniqueAssignedZones.Contains(
				TWeakObjectPtr<AGP_EnemySpawnVolume>(Zone)))
		{
			continue;
		}

		OutAssignedZones.Add(Zone);
		const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (State && State->bCompleted)
		{
			++CompletedAssignedZoneCount;
		}
	}

	return GPRunProgressionPolicy::IsOuterStageReady(
		ActivePlayerCount,
		OutAssignedZones.Num(),
		CompletedAssignedZoneCount);
}

bool AGP_GameMode::TryGetIncompleteAssignedOuterRecoveryTransform(
	APlayerState* PlayerState,
	FTransform& OutRecoveryTransform) const
{
	OutRecoveryTransform = FTransform::Identity;
	if (!bUseStagedZoneProgression || !IsValid(PlayerState) || !IsValid(GetWorld()))
	{
		return false;
	}

	const TWeakObjectPtr<AGP_EnemySpawnVolume>* AssignedZonePtr =
		AssignedOuterZonesByPlayer.Find(
			TWeakObjectPtr<APlayerState>(PlayerState));
	AGP_EnemySpawnVolume* AssignedZone =
		AssignedZonePtr ? AssignedZonePtr->Get() : nullptr;
	const FGPZoneRuntimeState* AssignedState =
		IsValid(AssignedZone)
			? ZoneRuntimeStates.Find(
				TWeakObjectPtr<AGP_EnemySpawnVolume>(AssignedZone))
			: nullptr;
	if (!IsValid(AssignedZone)
		|| AssignedZone->GetZoneStage() != EGPZoneStage::Outer
		|| !AssignedState
		|| AssignedState->bCompleted)
	{
		return false;
	}

	APlayerStart* MatchingStart = nullptr;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* CandidateStart = *It;
		if (!IsValid(CandidateStart)
			|| CandidateStart->GetLevel() != AssignedZone->GetLevel())
		{
			continue;
		}

		if (!MatchingStart
			|| CandidateStart->GetFName().LexicalLess(
				MatchingStart->GetFName()))
		{
			MatchingStart = CandidateStart;
		}
	}

	if (!IsValid(MatchingStart))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerLife] Incomplete assigned Outer '%s' has no PlayerStart for '%s'."),
			*AssignedZone->GetZoneId().ToString(),
			*GetNameSafe(PlayerState));
		return false;
	}

	OutRecoveryTransform = MatchingStart->GetActorTransform();
	UE_LOG(LogTemp, Log,
		TEXT("[PlayerLife] Player '%s' will recover at incomplete assigned Outer '%s'."),
		*GetNameSafe(PlayerState),
		*AssignedZone->GetZoneId().ToString());
	return true;
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
	if (!World || !IsValid(FromZone))
	{
		return;
	}
	if (!*PortalClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Cannot spawn Middle selection portal for zone '%s': PortalClass is not configured or was not cooked."),
			*GetNameSafe(FromZone));
		return;
	}

	const AActor* PortalAnchor =
		FindTaggedActorInZoneLevel(FromZone, VillagePortalAnchorTag);
	const FVector SpawnLocation =
		PortalAnchor ? PortalAnchor->GetActorLocation() : PreferredSpawnLocation;

	const FTransform SpawnTransform(
		PortalAnchor
			? PortalAnchor->GetActorRotation()
			: FRotator::ZeroRotator,
		SpawnLocation);
	if (AGP_RunPortal* Portal = World->SpawnActorDeferred<AGP_RunPortal>(
		PortalClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Portal->SetMiddleDestinationSelection(true);
		UGameplayStatics::FinishSpawningActor(Portal, SpawnTransform);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Spawned Middle selection portal '%s' for active Outer zone '%s' at %s."),
			*Portal->GetName(),
			*FromZone->GetZoneId().ToString(),
			*SpawnLocation.ToCompactString());
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] SpawnActor failed for Middle selection portal in Outer zone '%s' using class '%s'."),
			*FromZone->GetZoneId().ToString(),
			*GetNameSafe(PortalClass.Get()));
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

AGP_EnemySpawnVolume* AGP_GameMode::FindJoinableColosseumZone() const
{
	AGP_EnemySpawnVolume* IntroZone = nullptr;
	for (AGP_EnemySpawnVolume* Zone : OrderedZones)
	{
		if (!IsValid(Zone)
			|| Zone->GetZoneStage() != EGPZoneStage::Colosseum)
		{
			continue;
		}

		const FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
		if (!State
			|| !GPRunProgressionPolicy::ShouldRelocateJoiningPlayerToZone(
				Zone->GetZoneStage(),
				State->bIntroStarted,
				State->bIntroCompleted,
				State->bStarted,
				State->bCompleted))
		{
			continue;
		}

		if (State->bStarted)
		{
			return Zone;
		}
		if (!IntroZone
			&& (State->bIntroStarted || State->bIntroCompleted))
		{
			IntroZone = Zone;
		}
	}
	return IntroZone;
}

bool AGP_GameMode::TryRelocatePlayerToActiveColosseum(
	AController* PlayerController)
{
	if (!HasAuthority()
		|| bRunFinished
		|| !bUseStagedZoneProgression
		|| !IsValid(PlayerController)
		|| !IsValid(GetWorld()))
	{
		return false;
	}

	UWorld* World = GetWorld();
	const TWeakObjectPtr<AController> ControllerKey(PlayerController);
	const auto ClearRetryState = [this, World, ControllerKey]()
	{
		if (FTimerHandle* RetryTimer =
			ColosseumRelocationRetryTimers.Find(ControllerKey))
		{
			World->GetTimerManager().ClearTimer(*RetryTimer);
		}
		ColosseumRelocationRetryTimers.Remove(ControllerKey);
		ColosseumRelocationRetryCounts.Remove(ControllerKey);
	};

	AGP_EnemySpawnVolume* Zone = FindJoinableColosseumZone();
	APawn* PlayerPawn = PlayerController->GetPawn();
	APlayerState* PlayerState =
		PlayerController->GetPlayerState<APlayerState>();
	if (!IsValid(Zone)
		|| !IsValid(PlayerPawn)
		|| !IsValid(PlayerState))
	{
		ClearRetryState();
		return false;
	}

	FGPZoneRuntimeState* State = ZoneRuntimeStates.Find(Zone);
	if (!State)
	{
		ClearRetryState();
		return false;
	}

	const auto ScheduleRetry =
		[this, World, ControllerKey, Zone]() -> bool
	{
		if (const FTimerHandle* ExistingTimer =
				ColosseumRelocationRetryTimers.Find(ControllerKey);
			ExistingTimer
				&& World->GetTimerManager().IsTimerActive(*ExistingTimer))
		{
			return true;
		}

		const float RetryInterval =
			FMath::Max(0.05f, ZoneNavigationRetryInterval);
		const int32 WarningRetryCount = FMath::Max(
			1,
			FMath::CeilToInt(
				ZoneNavigationReadyTimeout / RetryInterval));
		int32& RetryCount =
			ColosseumRelocationRetryCounts.FindOrAdd(ControllerKey);
		if (RetryCount == 0)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[GP_GameMode] Joining player is waiting for a safe arrival point in active Colosseum zone '%s'."),
				*Zone->GetZoneId().ToString());
		}
		else if (RetryCount == WarningRetryCount)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[GP_GameMode] Active Colosseum zone '%s' still has no safe arrival after %.1f seconds; relocation retries continue."),
				*Zone->GetZoneId().ToString(),
				ZoneNavigationReadyTimeout);
		}
		RetryCount =
			FMath::Min(RetryCount + 1, WarningRetryCount + 1);

		const TWeakObjectPtr<AController> WeakController =
			ControllerKey;
		FTimerHandle& RetryTimer =
			ColosseumRelocationRetryTimers.FindOrAdd(ControllerKey);
		World->GetTimerManager().SetTimer(
			RetryTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, WeakController]()
				{
					ColosseumRelocationRetryTimers.Remove(
						WeakController);
					if (bRunFinished || !WeakController.IsValid())
					{
						ColosseumRelocationRetryCounts.Remove(
							WeakController);
						return;
					}

					if (!TryRelocatePlayerToActiveColosseum(
						WeakController.Get()))
					{
						// The encounter may have ended while navigation was
						// pending. Resume the ordinary Outer assignment path.
						AssignPlayersToOuterVillageStarts();
					}
				}),
			RetryInterval,
			false);
		return true;
	};

	bool bProjected = false;
	const FVector ArrivalLocation =
		Zone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);
	if (!bProjected)
	{
		return ScheduleRetry();
	}

	const FVector SafeArrival =
		ArrivalLocation
		+ FVector(
			0.0f,
			0.0f,
			PlayerPawn->GetSimpleCollisionHalfHeight() + 10.0f);
	if (!PlayerPawn->TeleportTo(
		SafeArrival,
		PlayerPawn->GetActorRotation(),
		false,
		true))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Failed to relocate joining player '%s' to active Colosseum zone '%s'."),
			*GetNameSafe(PlayerState),
			*Zone->GetZoneId().ToString());
		return ScheduleRetry();
	}

	ClearRetryState();
	if (AGP_PlayerController* GPPlayerController =
		Cast<AGP_PlayerController>(PlayerController))
	{
		// Reconnects enter this gameplay world behind the same initial loading
		// gate as Outer placement. Complete that gate at the authoritative
		// Colosseum destination instead of leaving input permanently blocked.
		GPPlayerController->ClientAwaitInitialOuterPlacement(
			PlayerPawn->GetActorLocation());
	}

	// The original portal admission already started the intro. Give a joining
	// PlayerState equivalent server-side admission so it can participate rather
	// than respawning at an Outer village after the portal has closed.
	State->PortalArrivals.Add(PlayerState);
	HandlePlayerEnteredZoneDetailed(Zone, PlayerState);
	UE_LOG(LogTemp, Log,
		TEXT("[GP_GameMode] Relocated joining player '%s' to active Colosseum zone '%s'."),
		*GetNameSafe(PlayerState),
		*Zone->GetZoneId().ToString());
	return true;
}

void AGP_GameMode::SpawnPortalBetweenZones(
	AGP_EnemySpawnVolume* FromZone,
	AGP_EnemySpawnVolume* ToZone,
	const FVector& PreferredSpawnLocation,
	int32 RetryCount)
{
	UWorld* World = GetWorld();
	if (bRunFinished
		|| !World
		|| !IsValid(FromZone)
		|| !IsValid(ToZone))
	{
		return;
	}
	if (!*PortalClass)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Cannot spawn staged portal from '%s' to '%s': PortalClass is not configured or was not cooked."),
			*GetNameSafe(FromZone),
			*GetNameSafe(ToZone));
		return;
	}

	bool bTargetProjected = false;
	const FVector TargetPosition =
		ToZone->GetSpawnPoint(/*bRandomizeInVolume=*/false, bTargetProjected);
	if (!bTargetProjected)
	{
		const float RetryInterval = FMath::Max(0.05f, ZoneNavigationRetryInterval);
		const int32 WarningRetryCount = FMath::Max(
			1,
			FMath::CeilToInt(ZoneNavigationReadyTimeout / RetryInterval));
		if (bUseZoneNavigationInvokers)
		{
			if (RetryCount == 0)
			{
				UE_LOG(LogTemp, Warning,
					TEXT("[GP_GameMode] Staged portal to zone '%s' is waiting for a safe NavMesh destination."),
					*ToZone->GetZoneId().ToString());
			}
			else if (RetryCount == WarningRetryCount)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[GP_GameMode] Staged portal to zone '%s' still has no safe destination after %.1f seconds; retries continue."),
					*ToZone->GetZoneId().ToString(),
					ZoneNavigationReadyTimeout);
			}

			const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakFromZone = FromZone;
			const TWeakObjectPtr<AGP_EnemySpawnVolume> WeakToZone = ToZone;
			const int32 NextRetryCount =
				FMath::Min(RetryCount + 1, WarningRetryCount + 1);
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(
				RetryTimer,
				FTimerDelegate::CreateWeakLambda(
					this,
					[this, WeakFromZone, WeakToZone, PreferredSpawnLocation, NextRetryCount]()
					{
						if (!bRunFinished
							&& WeakFromZone.IsValid()
							&& WeakToZone.IsValid())
						{
							SpawnPortalBetweenZones(
								WeakFromZone.Get(),
								WeakToZone.Get(),
								PreferredSpawnLocation,
								NextRetryCount);
						}
					}),
				RetryInterval,
				false);
			return;
		}

		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Skipped staged portal to zone '%s': no NavMesh destination exists and zone navigation invokers are disabled."),
			*ToZone->GetZoneId().ToString());
		return;
	}

	const FTransform SpawnTransform(
		FRotator::ZeroRotator,
		PreferredSpawnLocation);
	if (AGP_RunPortal* Portal = World->SpawnActorDeferred<AGP_RunPortal>(
		PortalClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		Portal->SetDestination(ToZone, TargetPosition);
		UGameplayStatics::FinishSpawningActor(Portal, SpawnTransform);
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Spawned staged portal '%s' from '%s' to '%s'."),
			*Portal->GetName(),
			*FromZone->GetZoneId().ToString(),
			*ToZone->GetZoneId().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] SpawnActor failed for staged portal from '%s' to '%s' using class '%s'."),
			*FromZone->GetZoneId().ToString(),
			*ToZone->GetZoneId().ToString(),
			*GetNameSafe(PortalClass.Get()));
	}
}

void AGP_GameMode::AssignPlayersToOuterVillageStarts()
{
	if (!bUseStagedZoneProgression || !GetWorld())
	{
		return;
	}
	if (FindJoinableColosseumZone())
	{
		// A late visual-ready callback must not pull a player back to their old
		// Outer start after RestartPlayer admitted them to the active boss room.
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
	TArray<AGP_EnemySpawnVolume*> OuterZonesWithStarts;
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
			OuterZonesWithStarts.Add(Zone);
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

	const int32 LayoutRevision = IsValid(VillageLayoutDirector)
		? VillageLayoutDirector->GetRuntimeLayoutRevision()
		: 0;
	const bool bRequiresClientVisualReady =
		IsValid(VillageLayoutDirector) && LayoutRevision > 0;
	bool bOuterAssignmentsChanged = false;
	int32 ActivePlayerCount = 0;
	for (APlayerController* PlayerController : Players)
	{
		APlayerState* PlayerState =
			IsValid(PlayerController)
				? PlayerController->GetPlayerState<APlayerState>()
				: nullptr;
		if (!IsValid(PlayerController)
			|| !IsValid(PlayerState)
			|| PlayerState->IsOnlyASpectator())
		{
			continue;
		}
		++ActivePlayerCount;

		// Reuse the stable party slot assigned at login instead of the player's
		// current sorted-array index. This prevents a reconnect from sharing an
		// occupied outer village after another player leaves.
		const int32 PartySlotIndex = ResolvePartyStartSlot(PlayerController);
		if (!OuterStarts.IsValidIndex(PartySlotIndex)
			|| !OuterZonesWithStarts.IsValidIndex(PartySlotIndex))
		{
			continue;
		}

		const TWeakObjectPtr<APlayerState> PlayerStateKey(PlayerState);
		AGP_EnemySpawnVolume* AssignedOuterZone =
			OuterZonesWithStarts[PartySlotIndex];
		const TWeakObjectPtr<AGP_EnemySpawnVolume>* ExistingOuterZone =
			AssignedOuterZonesByPlayer.Find(PlayerStateKey);
		if (!ExistingOuterZone || ExistingOuterZone->Get() != AssignedOuterZone)
		{
			AssignedOuterZonesByPlayer.Add(PlayerStateKey, AssignedOuterZone);
			bOuterAssignmentsChanged = true;
			UE_LOG(LogTemp, Log,
				TEXT("[GP_GameMode] Assigned active player %d to required Outer zone '%s'."),
				PartySlotIndex,
				*AssignedOuterZone->GetZoneId().ToString());
		}

		const TWeakObjectPtr<APlayerController> PlayerKey(PlayerController);
		const int32* ReadyRevision = VillageVisualReadyRevisions.Find(PlayerKey);
		const bool bVisualsReady = !bRequiresClientVisualReady
			|| PlayerController->IsLocalController()
			|| (ReadyRevision && *ReadyRevision == LayoutRevision);
		if (!bVisualsReady)
		{
			UE_LOG(LogTemp, Log,
				TEXT("[GP_GameMode] Waiting to assign '%s' until village visuals are ready for Revision=%d."),
				*PlayerController->GetName(),
				LayoutRevision);
			continue;
		}

		const int32* AssignedRevision = OuterAssignmentRevisions.Find(PlayerKey);
		if (AssignedRevision && *AssignedRevision == LayoutRevision)
		{
			continue;
		}

		APawn* Pawn = PlayerController->GetPawn();
		APlayerStart* PlayerStart = OuterStarts[PartySlotIndex];
		if (!IsValid(Pawn) || !IsValid(PlayerStart))
		{
			continue;
		}

		const bool bTeleported = Pawn->TeleportTo(
			PlayerStart->GetActorLocation(),
			PlayerStart->GetActorRotation(),
			false,
			true);
		if (!bTeleported)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[GP_GameMode] Failed to teleport player %d to outer start '%s'; assignment revision was not committed."),
				PartySlotIndex,
				*PlayerStart->GetName());
			continue;
		}
		OuterAssignmentRevisions.Add(PlayerKey, LayoutRevision);
		if (AGP_PlayerController* GPPlayerController =
			Cast<AGP_PlayerController>(PlayerController))
		{
			GPPlayerController->ClientAwaitInitialOuterPlacement(
				Pawn->GetActorLocation());
		}
		UE_LOG(LogTemp, Log,
			TEXT("[GP_GameMode] Assigned player %d to outer start '%s' after visual Revision=%d."),
			PartySlotIndex,
			*PlayerStart->GetName(),
			LayoutRevision);
	}

	if (OuterStarts.Num() < ActivePlayerCount)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[GP_GameMode] Found %d active player(s) but only %d streamed Outer PlayerStart(s); verify Small Outer presets."),
			ActivePlayerCount,
			OuterStarts.Num());
	}

	// This also covers a zone that completed before a late/restarted player's
	// streamed village assignment became available.
	if (bOuterAssignmentsChanged)
	{
		EvaluateStagedProgression();
	}
}

AGP_GameState* AGP_GameMode::GetGPGameState() const
{
	return GetGameState<AGP_GameState>();
}
