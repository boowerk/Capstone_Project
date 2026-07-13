#include "Game/RegionEvents/GP_RegionEventDirector.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameState.h"
#include "Game/RegionEvents/GP_CrystalCorruptionRegionEventActor.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventExplorationPolicy.h"
#include "Game/RegionEvents/GP_RegionEventSelectionPolicy.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"
#include "Game/Regions/GP_RegionSpatialSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformTime.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	TSubclassOf<AGP_RegionEventActor> ResolveProductionEventClass(const UGP_RegionEventData& EventData)
	{
		const UClass* ConfiguredClass = EventData.EventActorClass.Get();
		if (IsValid(ConfiguredClass) && ConfiguredClass->IsChildOf(AGP_RedRiftRegionEventActor::StaticClass()))
		{
			return AGP_RedRiftRegionEventActor::StaticClass();
		}
		if (IsValid(ConfiguredClass) && ConfiguredClass->IsChildOf(AGP_CrystalCorruptionRegionEventActor::StaticClass()))
		{
			return AGP_CrystalCorruptionRegionEventActor::StaticClass();
		}
		if (IsValid(ConfiguredClass) && ConfiguredClass->IsChildOf(AGP_ShrineRuinsRegionEventActor::StaticClass()))
		{
			return AGP_ShrineRuinsRegionEventActor::StaticClass();
		}
		if (IsValid(ConfiguredClass) && ConfiguredClass->IsChildOf(AGP_StructureDefenseRegionEventActor::StaticClass()))
		{
			return AGP_StructureDefenseRegionEventActor::StaticClass();
		}

		return EventData.EventActorClass;
	}

	float GetDefaultExplorationCompletionReduction(EGPRegionEventType EventType)
	{
		// Hostile objectives cleanse more corruption than the low-risk shrine reward.
		switch (EventType)
		{
		case EGPRegionEventType::CorruptionRift:
			return 14.0f;
		case EGPRegionEventType::EnvironmentalHazard:
			return 12.0f;
		case EGPRegionEventType::EnemyAmbush:
			return 10.0f;
		case EGPRegionEventType::ShrineCache:
		default:
			return 7.0f;
		}
	}
}

AGP_RegionEventDirector::AGP_RegionEventDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RegionEventActorClass = AGP_RegionEventActor::StaticClass();

	// World-event data is isolated from the designer's existing map EventPool and can still be overridden in BP.
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> RedRiftDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_RedRift.DA_RE_World_RedRift"));
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> CrystalDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_CrystalCorruption.DA_RE_World_CrystalCorruption"));
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> ShrineDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_ShrineRuins.DA_RE_World_ShrineRuins"));
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> DefenseDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_StructureDefense.DA_RE_World_StructureDefense"));

	if (RedRiftDataFinder.Succeeded())
	{
		ExplorationEventPool.Add(RedRiftDataFinder.Object);
	}
	if (CrystalDataFinder.Succeeded())
	{
		ExplorationEventPool.Add(CrystalDataFinder.Object);
	}
	if (ShrineDataFinder.Succeeded())
	{
		ExplorationEventPool.Add(ShrineDataFinder.Object);
	}
	if (DefenseDataFinder.Succeeded())
	{
		ExplorationEventPool.Add(DefenseDataFinder.Object);
	}
}

void AGP_RegionEventDirector::BeginPlay()
{
	Super::BeginPlay();

	// Standalone placement still works if GameMode has not supplied the authored seed count yet.
	if (HasAuthority() && !bDirectorInitialized)
	{
		InitializeRegionEventDirector(RegionCount);
	}
}

void AGP_RegionEventDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopExplorationScheduler();
	Super::EndPlay(EndPlayReason);
}

void AGP_RegionEventDirector::InitializeRegionEventDirector(int32 InRegionCount)
{
	if (!HasAuthority())
	{
		return;
	}

	RegionCount = FMath::Max(0, InRegionCount);
	const int32 RuntimeSeed = bUseDeterministicRandomSeed
		? RandomSeed
		: RandomSeed ^ static_cast<int32>(FPlatformTime::Cycles());
	EventRandomStream.Initialize(RuntimeSeed);
	bDirectorInitialized = true;
	ObservedRegionId = INDEX_NONE;
	ObservedRegionSinceSeconds = 0.0;
	RegionCooldownUntilSeconds.Reset();
	GlobalExplorationCooldownUntilSeconds = 0.0;
	LastExplorationEventId = NAME_None;
	RemoveInactiveEvents();

	const UWorld* World = GetWorld();
	NextExplorationAttemptSeconds = World
		? World->GetTimeSeconds() + FMath::Max(0.0f, InitialExplorationDelaySeconds)
		: 0.0;
	StartExplorationScheduler();
}

AGP_RegionEventActor* AGP_RegionEventDirector::TryStartRegionEventForZone(
	AGP_EnemySpawnVolume* Zone,
	EGPRegionEventTrigger Trigger)
{
	if (!HasAuthority() || !bEnableRegionEvents || !IsValid(Zone))
	{
		return nullptr;
	}

	const int32 RegionId = ResolveRegionIdForZone(Zone);
	if (RegionId == INDEX_NONE)
	{
		return nullptr;
	}

	const UGP_RegionEventData* SelectedEvent = GPRegionEventSelectionPolicy::SelectWeightedEvent(
		EventPool,
		Trigger,
		EventRandomStream);
	return SpawnSelectedEvent(RegionId, ResolveSpawnLocationForZone(Zone), SelectedEvent, false);
}

AGP_RegionEventActor* AGP_RegionEventDirector::TryStartRegionEventAtLocation(
	int32 RegionId,
	FVector SpawnLocation,
	EGPRegionEventTrigger Trigger)
{
	if (!HasAuthority() || !bEnableRegionEvents)
	{
		return nullptr;
	}

	const UGP_RegionEventData* SelectedEvent = GPRegionEventSelectionPolicy::SelectWeightedEvent(
		EventPool,
		Trigger,
		EventRandomStream);
	return SpawnSelectedEvent(RegionId, SpawnLocation + SpawnOffset, SelectedEvent, false);
}

AGP_RegionEventActor* AGP_RegionEventDirector::SpawnSelectedEvent(
	int32 RegionId,
	const FVector& SpawnLocation,
	const UGP_RegionEventData* SelectedEvent,
	bool bExplorationEvent)
{
	if (!HasAuthority() || !bEnableRegionEvents || !IsValid(SelectedEvent))
	{
		return nullptr;
	}

	RemoveInactiveEvents();
	if ((MaxActiveEvents >= 0 && ActiveEvents.Num() >= MaxActiveEvents)
		|| RegionId < 0
		|| (RegionCount > 0 && RegionId >= RegionCount)
		|| HasActiveEventForRegion(RegionId))
	{
		return nullptr;
	}

	TSubclassOf<AGP_RegionEventActor> ActorClassToSpawn = bExplorationEvent
		? ResolveProductionEventClass(*SelectedEvent)
		: SelectedEvent->EventActorClass;
	if (!*ActorClassToSpawn)
	{
		ActorClassToSpawn = RegionEventActorClass;
	}
	if (!*ActorClassToSpawn)
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
		ActorClassToSpawn,
		SpawnLocation,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!IsValid(EventActor))
	{
		return nullptr;
	}

	EventActor->OnRegionEventStateChanged.AddDynamic(this, &ThisClass::HandleRegionEventStateChanged);
	EventActor->OnRegionEventEnemySpawned.AddDynamic(this, &ThisClass::HandleRegionEventEnemySpawned);
	EventActor->InitializeRegionEvent(RegionId, const_cast<UGP_RegionEventData*>(SelectedEvent));
	if (bExplorationEvent)
	{
		// Exploration markers wait for the party, giving dynamic navigation time to build before enemies spawn.
		const float ActivationRadius = FMath::Max(100.0f, SelectedEvent->ApproachActivationRadius);
		const float DormantTimeout = SelectedEvent->DormantWaitTimeoutSeconds > 0.0f
			? SelectedEvent->DormantWaitTimeoutSeconds
			: 55.0f;
		EventActor->ConfigureExplorationActivation(ActivationRadius, DormantTimeout);
	}
	ActiveEvents.Add(EventActor);
	if (bExplorationEvent)
	{
		ExplorationEvents.Add(EventActor);
	}

	OnRegionEventStarted.Broadcast(this, EventActor);
	if (!bExplorationEvent && bAutoActivateSpawnedEvents)
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

	// Completion broadcasts can mutate ActiveEvents, so iterate a stable copy rather than the live array.
	const TArray<TObjectPtr<AGP_RegionEventActor>> EventsToComplete = ActiveEvents;
	for (AGP_RegionEventActor* EventActor : EventsToComplete)
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

bool AGP_RegionEventDirector::HasBlockingEventForRegion(int32 RegionId) const
{
	for (const AGP_RegionEventActor* EventActor : ActiveEvents)
	{
		const UGP_RegionEventData* EventData = IsValid(EventActor) ? EventActor->GetEventData() : nullptr;
		if (IsValid(EventActor)
			&& IsValid(EventData)
			&& EventActor->GetRegionId() == RegionId
			&& EventData->bBlocksZoneCompletion
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
		{
			return true;
		}
	}

	return false;
}

bool AGP_RegionEventDirector::HasBlockingEventForZone(const AGP_EnemySpawnVolume* Zone) const
{
	return HasBlockingEventForRegion(ResolveRegionIdForZone(Zone));
}

int32 AGP_RegionEventDirector::ResolveRegionIdForZone(const AGP_EnemySpawnVolume* Zone) const
{
	if (!IsValid(Zone))
	{
		return INDEX_NONE;
	}

	// Enemy strength, event outcome, and minimap corruption must all refer to the same explicit region id.
	const int32 ExplicitCorruptionRegionId = Zone->GetCorruptionRegionId();
	if (ExplicitCorruptionRegionId >= 0 && (RegionCount <= 0 || ExplicitCorruptionRegionId < RegionCount))
	{
		return ExplicitCorruptionRegionId;
	}

	const TArray<int32>& RegionsToRevive = Zone->GetRegionsToRevive();
	if (RegionsToRevive.Num() > 0
		&& RegionsToRevive[0] >= 0
		&& (RegionCount <= 0 || RegionsToRevive[0] < RegionCount))
	{
		return RegionsToRevive[0];
	}

	if (UWorld* World = GetWorld())
	{
		if (UGP_RegionSpatialSubsystem* SpatialRegions = World->GetSubsystem<UGP_RegionSpatialSubsystem>())
		{
			const int32 SpatialRegionId = SpatialRegions->ResolveRegionIdAtLocation(Zone->GetActorLocation());
			if (SpatialRegionId != INDEX_NONE)
			{
				return SpatialRegionId;
			}
		}
	}

	// Legacy maps without region seeds retain their stable ZoneOrder fallback.
	const int32 FallbackRegionId = Zone->GetZoneOrder();
	return FallbackRegionId >= 0 && (RegionCount <= 0 || FallbackRegionId < RegionCount)
		? FallbackRegionId
		: INDEX_NONE;
}

FVector AGP_RegionEventDirector::ResolveSpawnLocationForZone(AGP_EnemySpawnVolume* Zone) const
{
	if (!IsValid(Zone))
	{
		return GetActorLocation() + SpawnOffset;
	}

	bool bProjected = false;
	const FVector ProjectedZoneLocation = Zone->GetSpawnPoint(false, bProjected);
	return (bProjected ? ProjectedZoneLocation : Zone->GetActorLocation()) + SpawnOffset;
}

void AGP_RegionEventDirector::StartExplorationScheduler()
{
	StopExplorationScheduler();
	if (!HasAuthority() || !bEnableRegionEvents || !bEnableExplorationEvents || ExplorationEventPool.IsEmpty())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float Interval = FMath::Max(0.5f, ExplorationEvaluationIntervalSeconds);
		World->GetTimerManager().SetTimer(
			ExplorationEvaluationTimerHandle,
			this,
			&ThisClass::EvaluateExplorationEvents,
			Interval,
			true,
			Interval);
	}
}

void AGP_RegionEventDirector::StopExplorationScheduler()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExplorationEvaluationTimerHandle);
	}
}

void AGP_RegionEventDirector::EvaluateExplorationEvents()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !World || !bEnableRegionEvents || !bEnableExplorationEvents)
	{
		return;
	}

	RemoveInactiveEvents();
	const double Now = World->GetTimeSeconds();
	if (Now < NextExplorationAttemptSeconds
		|| Now < GlobalExplorationCooldownUntilSeconds
		|| CountActiveExplorationEvents() >= FMath::Max(1, MaxActiveExplorationEvents))
	{
		return;
	}

	if (const AGP_GameState* GameState = World->GetGameState<AGP_GameState>())
	{
		const EGPMatchPhase MatchPhase = GameState->GetMatchPhase();
		if (MatchPhase == EGPMatchPhase::BossFight
			|| MatchPhase == EGPMatchPhase::Victory
			|| MatchPhase == EGPMatchPhase::Defeat)
		{
			return;
		}
	}

	int32 PartyRegionId = INDEX_NONE;
	APawn* ReferencePawn = nullptr;
	if (!ResolvePartyRegion(PartyRegionId, ReferencePawn) || !IsValid(ReferencePawn))
	{
		ObservedRegionId = INDEX_NONE;
		return;
	}

	if (ObservedRegionId != PartyRegionId)
	{
		ObservedRegionId = PartyRegionId;
		ObservedRegionSinceSeconds = Now;
		return;
	}

	if (Now - ObservedRegionSinceSeconds < FMath::Max(0.0f, RegionDwellSeconds)
		|| Now < RegionCooldownUntilSeconds.FindRef(PartyRegionId)
		|| HasActiveEventForRegion(PartyRegionId))
	{
		return;
	}

	const float NormalizedCorruption = GetRegionCorruptionNormalized(PartyRegionId);
	const float AttemptChance = GPRegionEventExplorationPolicy::CalculateAttemptChance(
		NormalizedCorruption,
		BaseExplorationChance,
		FullCorruptionChanceBonus);
	if (EventRandomStream.FRand() > AttemptChance)
	{
		// A failed roll retries on the next evaluation without resetting dwell, avoiding long invisible dead periods.
		return;
	}

	const UGP_RegionEventData* SelectedEvent = SelectExplorationEvent(NormalizedCorruption);
	FVector SpawnLocation = FVector::ZeroVector;
	if (!IsValid(SelectedEvent)
		|| !ResolveExplorationSpawnLocation(*ReferencePawn, PartyRegionId, SpawnLocation))
	{
		NextExplorationAttemptSeconds = Now + FMath::Max(1.0f, ExplorationEvaluationIntervalSeconds);
		return;
	}

	AGP_RegionEventActor* SpawnedEvent = SpawnSelectedEvent(
		PartyRegionId,
		SpawnLocation + SpawnOffset,
		SelectedEvent,
		true);
	if (!IsValid(SpawnedEvent))
	{
		return;
	}

	LastExplorationEventId = SelectedEvent->EventId;
	GlobalExplorationCooldownUntilSeconds = Now + FMath::Max(0.0f, GlobalExplorationCooldownSeconds);
	const float RegionCooldown = SelectedEvent->RegionCooldownSeconds > 0.0f
		? SelectedEvent->RegionCooldownSeconds
		: 150.0f;
	RegionCooldownUntilSeconds.Add(PartyRegionId, Now + RegionCooldown);
	ObservedRegionSinceSeconds = Now;

	UE_LOG(LogTemp, Log, TEXT("[RegionEvent] Exploration event '%s' discovered in region %d at corruption %.2f."),
		*SelectedEvent->EventId.ToString(),
		PartyRegionId,
		NormalizedCorruption);
}

bool AGP_RegionEventDirector::ResolvePartyRegion(int32& OutRegionId, APawn*& OutReferencePawn) const
{
	OutRegionId = INDEX_NONE;
	OutReferencePawn = nullptr;

	UWorld* World = GetWorld();
	UGP_RegionSpatialSubsystem* SpatialRegions = World
		? World->GetSubsystem<UGP_RegionSpatialSubsystem>()
		: nullptr;
	if (!IsValid(SpatialRegions) || SpatialRegions->GetSeedCount() <= 0)
	{
		return false;
	}

	TMap<int32, int32> PartyCountsByRegion;
	TMap<int32, APawn*> ReferencePawnByRegion;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PlayerController = Iterator->Get();
		APawn* PlayerPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
		if (!IsValid(PlayerPawn))
		{
			continue;
		}

		const int32 PlayerRegionId = SpatialRegions->ResolveRegionIdAtLocation(PlayerPawn->GetActorLocation());
		if (PlayerRegionId == INDEX_NONE)
		{
			continue;
		}

		PartyCountsByRegion.FindOrAdd(PlayerRegionId)++;
		ReferencePawnByRegion.FindOrAdd(PlayerRegionId) = PlayerPawn;
	}

	int32 BestPlayerCount = 0;
	for (const TPair<int32, int32>& RegionCountPair : PartyCountsByRegion)
	{
		if (RegionCountPair.Value > BestPlayerCount
			|| (RegionCountPair.Value == BestPlayerCount && RegionCountPair.Key < OutRegionId))
		{
			OutRegionId = RegionCountPair.Key;
			BestPlayerCount = RegionCountPair.Value;
		}
	}

	OutReferencePawn = ReferencePawnByRegion.FindRef(OutRegionId);
	return OutRegionId != INDEX_NONE && IsValid(OutReferencePawn);
}

bool AGP_RegionEventDirector::ResolveExplorationSpawnLocation(
	const APawn& ReferencePawn,
	int32 RegionId,
	FVector& OutSpawnLocation)
{
	UWorld* World = GetWorld();
	UGP_RegionSpatialSubsystem* SpatialRegions = World
		? World->GetSubsystem<UGP_RegionSpatialSubsystem>()
		: nullptr;
	if (!World || !IsValid(SpatialRegions))
	{
		return false;
	}

	const float MinimumDistance = FMath::Max(100.0f, MinimumExplorationSpawnDistance);
	const float MaximumDistance = FMath::Max(MinimumDistance, MaximumExplorationSpawnDistance);
	const FVector Forward2D = ReferencePawn.GetActorForwardVector().GetSafeNormal2D();
	const FVector SafeForward = Forward2D.IsNearlyZero() ? FVector::ForwardVector : Forward2D;

	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float AngleDegrees = EventRandomStream.FRandRange(-75.0f, 75.0f);
		const float Distance = EventRandomStream.FRandRange(MinimumDistance, MaximumDistance);
		const FVector Direction = SafeForward.RotateAngleAxis(AngleDegrees, FVector::UpVector);
		const FVector DesiredLocation = ReferencePawn.GetActorLocation() + Direction * Distance;

		FVector CandidateLocation = DesiredLocation;
		bool bFoundGround = false;
		if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World))
		{
			FNavLocation ProjectedLocation;
			if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, ExplorationNavProjectionExtent))
			{
				CandidateLocation = ProjectedLocation.Location;
				bFoundGround = true;
			}
		}

		if (!bFoundGround)
		{
			FHitResult GroundHit;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RegionEventPlacement), false, &ReferencePawn);
			const FVector TraceStart = DesiredLocation + FVector(0.0f, 0.0f, 6000.0f);
			const FVector TraceEnd = DesiredLocation - FVector(0.0f, 0.0f, 10000.0f);
			if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
			{
				CandidateLocation = GroundHit.ImpactPoint;
				bFoundGround = true;
			}
		}

		if (bFoundGround && SpatialRegions->ResolveRegionIdAtLocation(CandidateLocation) == RegionId)
		{
			OutSpawnLocation = CandidateLocation;
			return true;
		}
	}

	return false;
}

const UGP_RegionEventData* AGP_RegionEventDirector::SelectExplorationEvent(float NormalizedCorruption)
{
	return GPRegionEventExplorationPolicy::SelectWeightedExplorationEvent(
		ExplorationEventPool,
		NormalizedCorruption,
		LastExplorationEventId,
		EventRandomStream);
}

int32 AGP_RegionEventDirector::CountActiveExplorationEvents() const
{
	int32 ActiveCount = 0;
	for (const TWeakObjectPtr<AGP_RegionEventActor>& EventActorPointer : ExplorationEvents)
	{
		const AGP_RegionEventActor* EventActor = EventActorPointer.Get();
		if (IsValid(EventActor)
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
		{
			++ActiveCount;
		}
	}
	return ActiveCount;
}

float AGP_RegionEventDirector::GetRegionCorruptionNormalized(int32 RegionId) const
{
	const AGP_GameState* GameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	const UGP_WorldCorruptionComponent* Corruption = IsValid(GameState)
		? GameState->GetWorldCorruptionComponent()
		: nullptr;
	return IsValid(Corruption) ? Corruption->GetRegionCorruptionNormalized(RegionId) : 0.0f;
}

void AGP_RegionEventDirector::ApplyEventCorruptionOutcome(AGP_RegionEventActor& EventActor)
{
	const TWeakObjectPtr<AGP_RegionEventActor> EventKey(&EventActor);
	if (AppliedOutcomeEvents.Contains(EventKey))
	{
		return;
	}

	UGP_RegionEventData* EventData = EventActor.GetEventData();
	AGP_GameState* GameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	UGP_WorldCorruptionComponent* Corruption = IsValid(GameState)
		? GameState->GetWorldCorruptionComponent()
		: nullptr;
	if (!IsValid(EventData) || !IsValid(Corruption))
	{
		return;
	}

	const bool bExplorationEvent = ExplorationEvents.Contains(EventKey);
	if (EventActor.GetRuntimeState() == EGPRegionEventRuntimeState::Completed
		&& bExplorationEvent
		&& EventData->CompletionCorruptionReduction <= 0.0f)
	{
		// Production copies inherit zero to preserve old test assets, so exploration supplies a safe gameplay default.
		Corruption->ReduceRegionCorruption(
			EventActor.GetRegionId(),
			GetDefaultExplorationCompletionReduction(EventData->EventType));
	}
	else if (EventActor.GetRuntimeState() == EGPRegionEventRuntimeState::Expired
		&& bExplorationEvent
		&& EventData->ExpirationCorruptionIncrease <= 0.0f)
	{
		// Ignoring or failing a discovered event has a visible but recoverable regional consequence.
		Corruption->AddRegionCorruption(
			EventActor.GetRegionId(),
			6.0f);
	}

	// Non-zero DataAsset values are already applied by the event actor; marking prevents accidental double handling.
	AppliedOutcomeEvents.Add(EventKey);
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

	for (auto Iterator = ExplorationEvents.CreateIterator(); Iterator; ++Iterator)
	{
		const AGP_RegionEventActor* EventActor = Iterator->Get();
		if (!IsValid(EventActor)
			|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Completed
			|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Expired)
		{
			Iterator.RemoveCurrent();
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
		ApplyEventCorruptionOutcome(*EventActor);
		OnRegionEventEnded.Broadcast(this, EventActor);

		// Keep the replicated presentation alive briefly so completion VFX and UI can finish naturally.
		if (FinishedEventLifeSpan > 0.0f)
		{
			EventActor->SetLifeSpan(FinishedEventLifeSpan);
		}

		RemoveInactiveEvents();
	}
}

void AGP_RegionEventDirector::HandleRegionEventEnemySpawned(
	AGP_RegionEventActor* EventActor,
	AGP_EnemyCharacter* Enemy)
{
	OnRegionEventEnemySpawned.Broadcast(EventActor, Enemy);
}
