#include "Game/GP_GameMode.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Game/Corruption/GP_CorruptionPresentationActor.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunPortal.h"
#include "Game/Regions/GP_RegionSpatialSubsystem.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"
#include "TimerManager.h"

AGP_GameMode::AGP_GameMode()
{
	GameStateClass = AGP_GameState::StaticClass();
	RegionEventDirectorClass = AGP_RegionEventDirector::StaticClass();
	CorruptionPresentationClass = AGP_CorruptionPresentationActor::StaticClass();

	// Match the lobby's seamless transition so arriving clients are carried
	// into this map instead of being dropped during the load.
	bUseSeamlessTravel = true;
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

void AGP_GameMode::Logout(AController* Exiting)
{
	// Release the deterministic slot immediately so a reconnect does not wait for garbage collection.
	PartyStartSlotByController.Remove(TWeakObjectPtr<AController>(Exiting));
	Super::Logout(Exiting);
}

void AGP_GameMode::EnsurePartyPlayerStarts(AController* Player)
{
	if (bPartyPlayerStartsInitialized)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<APlayerStart*> AuthoredStarts;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->Tags.Contains(TEXT("GP.RuntimePartyStart")))
		{
			AuthoredStarts.Add(*It);
		}
	}
	AuthoredStarts.Sort([](const APlayerStart& Left, const APlayerStart& Right)
	{
		// Actor paths are serialized and provide deterministic slot order across server runs.
		return Left.GetPathName() < Right.GetPathName();
	});

	if (AuthoredStarts.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[NetworkSpawn] No authored PlayerStart is available for the three-player runtime expansion."));
		bPartyPlayerStartsInitialized = true;
		return;
	}

	for (APlayerStart* AuthoredStart : AuthoredStarts)
	{
		if (PartyPlayerStartSlots.Num() >= RequiredPartyPlayerStartCount)
		{
			break;
		}
		PartyPlayerStartSlots.Add(AuthoredStart);
	}

	const APlayerStart& Anchor = *AuthoredStarts[0];
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	const APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	static const FVector CandidateDirections[] =
	{
		FVector(0.0f, 1.0f, 0.0f),
		FVector(0.0f, -1.0f, 0.0f),
		FVector(1.0f, 0.0f, 0.0f),
		FVector(-1.0f, 0.0f, 0.0f),
		FVector(1.0f, 1.0f, 0.0f).GetSafeNormal(),
		FVector(1.0f, -1.0f, 0.0f).GetSafeNormal(),
		FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal(),
		FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal()
	};

	for (float RadiusMultiplier = 1.0f;
		PartyPlayerStartSlots.Num() < RequiredPartyPlayerStartCount && RadiusMultiplier <= 3.0f;
		RadiusMultiplier += 1.0f)
	{
		for (const FVector& Direction : CandidateDirections)
		{
			if (APlayerStart* RuntimeStart = SpawnRuntimePartyPlayerStart(
				Anchor,
				PawnToFit,
				Direction,
				RadiusMultiplier))
			{
				RuntimePartyPlayerStarts.Add(RuntimeStart);
				PartyPlayerStartSlots.Add(RuntimeStart);
			}

			if (PartyPlayerStartSlots.Num() >= RequiredPartyPlayerStartCount)
			{
				break;
			}
		}
	}

	bPartyPlayerStartsInitialized = true;
	if (PartyPlayerStartSlots.Num() < RequiredPartyPlayerStartCount)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[NetworkSpawn] Only %d/%d collision-safe party starts could be prepared; engine fallback remains enabled."),
			PartyPlayerStartSlots.Num(),
			RequiredPartyPlayerStartCount);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[NetworkSpawn] Prepared %d stable party starts (%d authored, %d runtime)."),
			PartyPlayerStartSlots.Num(),
			FMath::Min(AuthoredStarts.Num(), RequiredPartyPlayerStartCount),
			RuntimePartyPlayerStarts.Num());
	}
}

APlayerStart* AGP_GameMode::SpawnRuntimePartyPlayerStart(
	const APlayerStart& Anchor,
	const APawn* PawnToFit,
	const FVector& LocalDirection,
	float RadiusMultiplier)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const FVector WorldDirection =
		Anchor.GetActorForwardVector() * LocalDirection.X
		+ Anchor.GetActorRightVector() * LocalDirection.Y;
	FVector CandidateLocation =
		Anchor.GetActorLocation()
		+ WorldDirection.GetSafeNormal() * PartyPlayerStartSpacing * RadiusMultiplier;
	const FRotator CandidateRotation = Anchor.GetActorRotation();
	float MinimumWalkableNormalZ = 0.7f;
	if (const ACharacter* CharacterToFit = Cast<ACharacter>(PawnToFit))
	{
		if (const UCharacterMovementComponent* Movement = CharacterToFit->GetCharacterMovement())
		{
			MinimumWalkableNormalZ = Movement->GetWalkableFloorZ();
		}
	}

	const auto SnapToWalkableGround = [World, &Anchor, PawnToFit, MinimumWalkableNormalZ](FVector& InOutLocation)
	{
		FHitResult GroundHit;
		FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(GP_RuntimePartyStartGround), false);
		GroundQueryParams.AddIgnoredActor(&Anchor);
		const FVector TraceStart(InOutLocation.X, InOutLocation.Y, Anchor.GetActorLocation().Z + 1000.0f);
		const FVector TraceEnd(InOutLocation.X, InOutLocation.Y, Anchor.GetActorLocation().Z - 2000.0f);
		if (!World->LineTraceSingleByObjectType(
			GroundHit,
			TraceStart,
			TraceEnd,
			FCollisionObjectQueryParams(ECC_WorldStatic),
			GroundQueryParams)
			|| GroundHit.ImpactNormal.Z < MinimumWalkableNormalZ)
		{
			return false;
		}

		const float PawnHalfHeight = PawnToFit ? PawnToFit->GetDefaultHalfHeight() : 96.0f;
		InOutLocation.Z = GroundHit.ImpactPoint.Z + PawnHalfHeight + 2.0f;
		return true;
	};

	// Never register a start over void or on a slope the production character cannot walk on.
	if (!SnapToWalkableGround(CandidateLocation))
	{
		return nullptr;
	}

	if (PawnToFit
		&& World->EncroachingBlockingGeometry(PawnToFit, CandidateLocation, CandidateRotation)
		&& !World->FindTeleportSpot(PawnToFit, CandidateLocation, CandidateRotation))
	{
		return nullptr;
	}

	// FindTeleportSpot may move XY as well as Z; validate its final surface instead of trusting the first trace.
	if (!SnapToWalkableGround(CandidateLocation)
		|| (PawnToFit && World->EncroachingBlockingGeometry(PawnToFit, CandidateLocation, CandidateRotation)))
	{
		return nullptr;
	}

	const float MinimumSeparation = FMath::Max(150.0f, PartyPlayerStartSpacing * 0.6f);
	for (const TWeakObjectPtr<APlayerStart>& ExistingStart : PartyPlayerStartSlots)
	{
		if (ExistingStart.IsValid()
			&& FVector::DistSquared(ExistingStart->GetActorLocation(), CandidateLocation)
				< FMath::Square(MinimumSeparation))
		{
			return nullptr;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* RuntimeStart = World->SpawnActor<APlayerStart>(
		APlayerStart::StaticClass(),
		CandidateLocation,
		CandidateRotation,
		SpawnParameters);
	if (IsValid(RuntimeStart))
	{
		// PlayerStarts are server-only selection helpers and never need to replicate to clients.
		RuntimeStart->SetReplicates(false);
		RuntimeStart->Tags.AddUnique(TEXT("GP.RuntimePartyStart"));
	}
	return RuntimeStart;
}

int32 AGP_GameMode::ResolvePartyStartSlot(AController* Player)
{
	if (!IsValid(Player))
	{
		return INDEX_NONE;
	}

	const TWeakObjectPtr<AController> PlayerKey(Player);
	for (auto It = PartyStartSlotByController.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (const int32* ExistingSlot = PartyStartSlotByController.Find(PlayerKey))
	{
		return *ExistingSlot;
	}

	TSet<int32> UsedSlots;
	for (const TPair<TWeakObjectPtr<AController>, int32>& Assignment : PartyStartSlotByController)
	{
		if (Assignment.Key.IsValid())
		{
			UsedSlots.Add(Assignment.Value);
		}
	}

	for (int32 SlotIndex = 0; SlotIndex < PartyPlayerStartSlots.Num(); ++SlotIndex)
	{
		if (!UsedSlots.Contains(SlotIndex) && PartyPlayerStartSlots[SlotIndex].IsValid())
		{
			PartyStartSlotByController.Add(PlayerKey, SlotIndex);
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}

void AGP_GameMode::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	BeginThreePlayerGameplaySmokeProbe();
#endif

	// Resolve map-authored seeds before the director and corruption state consume RegionCount.
	ResolveRuntimeRegionConfiguration();
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

#if !UE_BUILD_SHIPPING
void AGP_GameMode::BeginThreePlayerGameplaySmokeProbe()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)
		|| !World->GetMapName().Contains(TEXT("L_LandscapeMap"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("LobbySmokeAutoReady")))
	{
		return;
	}

	ThreePlayerGameplaySmokeAttempts = 0;
	World->GetTimerManager().SetTimer(
		ThreePlayerGameplaySmokeTimerHandle,
		this,
		&ThisClass::TryThreePlayerGameplaySmokeProbe,
		0.25f,
		false);
}

void AGP_GameMode::TryThreePlayerGameplaySmokeProbe()
{
	++ThreePlayerGameplaySmokeAttempts;
	constexpr int32 ExpectedPlayers = 3;
	constexpr int32 MaxAttempts = 80;
	constexpr float MinimumPawnSeparation = 150.0f;

	TArray<APlayerController*> Controllers;
	TArray<APawn*> Pawns;
	TSet<APawn*> UniquePawns;
	int32 OwnedPawnCount = 0;
	int32 GameplayControllerCount = 0;
	int32 GameplayPawnCount = 0;
	int32 GameplayPlayerStateCount = 0;

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controller = It->Get();
			if (!IsValid(Controller))
			{
				continue;
			}

			Controllers.Add(Controller);
			GameplayControllerCount += Controller->IsA<AGP_PlayerController>() ? 1 : 0;
			GameplayPlayerStateCount += Controller->GetPlayerState<AGP_PlayerState>() ? 1 : 0;
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawns.Add(Pawn);
				UniquePawns.Add(Pawn);
				OwnedPawnCount += Pawn->GetController() == Controller ? 1 : 0;
				GameplayPawnCount += Pawn->IsA<AGP_PlayerCharacter>() ? 1 : 0;
			}
		}
	}

	float MinimumObservedSeparation = TNumericLimits<float>::Max();
	for (int32 FirstIndex = 0; FirstIndex < Pawns.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < Pawns.Num(); ++SecondIndex)
		{
			MinimumObservedSeparation = FMath::Min(
				MinimumObservedSeparation,
				FVector::Dist(Pawns[FirstIndex]->GetActorLocation(), Pawns[SecondIndex]->GetActorLocation()));
		}
	}
	if (Pawns.Num() < 2)
	{
		MinimumObservedSeparation = 0.0f;
	}

	const bool bGameplayReady =
		Controllers.Num() == ExpectedPlayers
		&& Pawns.Num() == ExpectedPlayers
		&& UniquePawns.Num() == ExpectedPlayers
		&& OwnedPawnCount == ExpectedPlayers
		&& GameplayControllerCount == ExpectedPlayers
		&& GameplayPawnCount == ExpectedPlayers
		&& GameplayPlayerStateCount == ExpectedPlayers
		&& MinimumObservedSeparation >= MinimumPawnSeparation;

	if (bGameplayReady)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[Network3PSmoke] server-gameplay-ready controllers=%d pawns=%d unique=%d owned=%d gamePC=%d gamePawn=%d gamePS=%d minSeparation=%.1f"),
			Controllers.Num(),
			Pawns.Num(),
			UniquePawns.Num(),
			OwnedPawnCount,
			GameplayControllerCount,
			GameplayPawnCount,
			GameplayPlayerStateCount,
			MinimumObservedSeparation);
		return;
	}

	if (ThreePlayerGameplaySmokeAttempts >= MaxAttempts)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Network3PSmoke] server-gameplay-timeout attempts=%d controllers=%d pawns=%d unique=%d owned=%d gamePC=%d gamePawn=%d gamePS=%d minSeparation=%.1f"),
			ThreePlayerGameplaySmokeAttempts,
			Controllers.Num(),
			Pawns.Num(),
			UniquePawns.Num(),
			OwnedPawnCount,
			GameplayControllerCount,
			GameplayPawnCount,
			GameplayPlayerStateCount,
			MinimumObservedSeparation);
		return;
	}

	GetWorldTimerManager().SetTimer(
		ThreePlayerGameplaySmokeTimerHandle,
		this,
		&ThisClass::TryThreePlayerGameplaySmokeProbe,
		0.25f,
		false);
}
#endif

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
		// A blocking objective re-evaluates zone completion as soon as it succeeds or expires.
		RegionEventDirector->OnRegionEventEnded.AddDynamic(this, &ThisClass::HandleRegionEventEnded);
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

	MaybeCompleteZone();
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
	const AGP_EnemySpawnVolume* CurrentZone = OrderedZones.IsValidIndex(CurrentZoneIndex)
		? OrderedZones[CurrentZoneIndex]
		: nullptr;
	const bool bWaitingForRegionObjective = RegionEventDirector
		&& IsValid(CurrentZone)
		&& RegionEventDirector->HasBlockingEventForZone(CurrentZone);
	if (bAllMarkersDone && AliveZoneEnemies == 0 && !bWaitingForRegionObjective)
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

	if (Enemy->OnEnemyDeathStarted.IsAlreadyBound(this, &AGP_GameMode::HandleZoneEnemyDied))
	{
		return;
	}

	// The explicit event/zone id lets enemies retain their regional strength even after progression advances.
	Enemy->SetCorruptionRegionId(CorruptionRegionId != INDEX_NONE ? CorruptionRegionId : CurrentZoneIndex);

	// The terminal death delegate also fires for scripted encounter cleanup, unlike the HP-zero reward delegate.
	Enemy->OnEnemyDeathStarted.AddDynamic(this, &AGP_GameMode::HandleZoneEnemyDied);
	++AliveZoneEnemies;

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}
}

void AGP_GameMode::HandleZoneEnemyDied(AGP_EnemyCharacter* DeadEnemy, AActor* DeathInstigator)
{
	AliveZoneEnemies = FMath::Max(0, AliveZoneEnemies - 1);

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetEnemiesRemaining(AliveZoneEnemies);
	}

	MaybeCompleteZone();
}

void AGP_GameMode::HandleRegionEventEnemySpawned(AGP_RegionEventActor* EventActor, AGP_EnemyCharacter* Enemy)
{
	if (RegionEventDirector && RegionEventDirector->IsExplorationEvent(EventActor))
	{
		// Open-world encounters own and retire their enemies; they must not mutate an unrelated linear-zone budget.
		return;
	}

	// Event-spawned enemies join the current zone budget so events cannot be ignored during a city clear.
	RegisterZoneEnemy(Enemy, IsValid(EventActor) ? EventActor->GetRegionId() : INDEX_NONE);
}

void AGP_GameMode::HandleRegionEventEnded(AGP_RegionEventDirector* Director, AGP_RegionEventActor* EventActor)
{
	(void)Director;
	(void)EventActor;
	// Enemy deaths and objective completion may arrive in either order; this single gate handles both safely.
	MaybeCompleteZone();
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
	const FVector SpawnPos = OrderedZones[FromZoneIndex]->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);
	const FVector TargetPos = OrderedZones[ToZoneIndex]->GetSpawnPoint(/*bRandomizeInVolume=*/false, bProjected);

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
