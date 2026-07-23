#include "Game/RegionEvents/GP_RegionEventDirector.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AGP_RegionEventDirector::AGP_RegionEventDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	RegionEventActorClass = AGP_RegionEventActor::StaticClass();

	// The graduation route has three authored beats. Loading them directly removes all content-pool randomness.
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> RedRiftDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_RedRift.DA_RE_World_RedRift"));
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> ShrineDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_ShrineRuins.DA_RE_World_ShrineRuins"));
	static ConstructorHelpers::FObjectFinder<UGP_RegionEventData> DefenseDataFinder(
		TEXT("/Game/RegionEvents/Runtime/DA_RE_World_StructureDefense.DA_RE_World_StructureDefense"));

	if (RedRiftDataFinder.Succeeded())
	{
		GuidedEventPool.Add(RedRiftDataFinder.Object);
	}
	if (ShrineDataFinder.Succeeded())
	{
		GuidedEventPool.Add(ShrineDataFinder.Object);
	}
	if (DefenseDataFinder.Succeeded())
	{
		GuidedEventPool.Add(DefenseDataFinder.Object);
	}
}

void AGP_RegionEventDirector::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && !bDirectorInitialized)
	{
		// A placed Blueprint remains usable even when GameMode initialization arrives one frame later.
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
	bDirectorInitialized = true;
	RemoveInactiveEvents();
}

AGP_RegionEventActor* AGP_RegionEventDirector::TryStartGuidedRegionEventAtLocation(
	FName EventId,
	int32 RegionId,
	FVector SpawnLocation,
	float DormantWaitTimeoutSeconds,
	int32 RequiredApproachPlayers)
{
	if (!HasAuthority() || !bEnableScriptedEvents)
	{
		return nullptr;
	}

	const UGP_RegionEventData* EventData = FindUniqueGuidedEventById(EventId);
	if (!IsValid(EventData))
	{
		return nullptr;
	}

	return SpawnGuidedEvent(
		RegionId,
		SpawnLocation + SpawnOffset,
		*EventData,
		FMath::Max(-1.0f, DormantWaitTimeoutSeconds),
		FMath::Max(1, RequiredApproachPlayers));
}

void AGP_RegionEventDirector::CompleteEventsForRegion(int32 RegionId)
{
	if (!HasAuthority())
	{
		return;
	}

	// Completion callbacks mutate ActiveEvents, so iterate a stable copy.
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

int32 AGP_RegionEventDirector::GetActiveEventCount() const
{
	int32 ActiveCount = 0;
	for (const AGP_RegionEventActor* EventActor : ActiveEvents)
	{
		// Delegate cleanup can trail a synchronous state change in editor-only worlds; public state stays exact.
		if (IsValid(EventActor)
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
			&& EventActor->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
		{
			++ActiveCount;
		}
	}
	return ActiveCount;
}

const UGP_RegionEventData* AGP_RegionEventDirector::FindUniqueGuidedEventById(FName EventId) const
{
	if (EventId.IsNone())
	{
		return nullptr;
	}

	const UGP_RegionEventData* Match = nullptr;
	for (const UGP_RegionEventData* Candidate : GuidedEventPool)
	{
		if (!IsValid(Candidate) || Candidate->EventId != EventId)
		{
			continue;
		}
		if (Match)
		{
			// Duplicate IDs fail closed so the fixed demo never depends on array ordering.
			UE_LOG(LogTemp, Warning, TEXT("[DemoEvent] EventId '%s' is duplicated."), *EventId.ToString());
			return nullptr;
		}
		Match = Candidate;
	}

	if (!IsValid(Match))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DemoEvent] EventId '%s' is unavailable."), *EventId.ToString());
	}
	return Match;
}

AGP_RegionEventActor* AGP_RegionEventDirector::SpawnGuidedEvent(
	int32 RegionId,
	const FVector& SpawnLocation,
	const UGP_RegionEventData& EventData,
	float DormantWaitTimeoutSeconds,
	int32 RequiredApproachPlayers)
{
	RemoveInactiveEvents();
	if (!HasAuthority()
		|| !bEnableScriptedEvents
		|| ActiveEvents.Num() >= FMath::Max(1, MaxActiveEvents)
		|| RegionId < 0
		|| (RegionCount > 0 && RegionId >= RegionCount)
		|| HasActiveEventForRegion(RegionId))
	{
		return nullptr;
	}

	TSubclassOf<AGP_RegionEventActor> ActorClassToSpawn = EventData.EventActorClass;
	if (!*ActorClassToSpawn)
	{
		ActorClassToSpawn = RegionEventActorClass;
	}
	UWorld* World = GetWorld();
	if (!World || !*ActorClassToSpawn)
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
	EventActor->InitializeRegionEvent(RegionId, const_cast<UGP_RegionEventData*>(&EventData));
	ActiveEvents.Add(EventActor);

	if (AGP_ShrineRuinsRegionEventActor* GuidedShrine = Cast<AGP_ShrineRuinsRegionEventActor>(EventActor))
	{
		// The fixed shrine rewards every connected party member after the shared approach gate.
		GuidedShrine->ConfigureGuidedPartyReward(true);
	}

	EventActor->ConfigureGuidedActivation(
		FMath::Max(100.0f, EventData.ApproachActivationRadius),
		DormantWaitTimeoutSeconds,
		RequiredApproachPlayers);
	OnRegionEventStarted.Broadcast(this, EventActor);
	return EventActor;
}

void AGP_RegionEventDirector::RemoveInactiveEvents()
{
	for (int32 Index = ActiveEvents.Num() - 1; Index >= 0; --Index)
	{
		const AGP_RegionEventActor* EventActor = ActiveEvents[Index];
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
		if (FinishedEventLifeSpan > 0.0f)
		{
			// Keep replicated completion presentation alive briefly before deterministic cleanup.
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
