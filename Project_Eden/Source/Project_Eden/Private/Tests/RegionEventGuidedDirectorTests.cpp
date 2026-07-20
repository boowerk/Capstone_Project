#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPRegionEventGuidedDirectorControlTest,
	"ProjectEden.Game.RegionEvents.GuidedDirectorControl",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPRegionEventGuidedDirectorControlTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("GuidedRegionEventDirectorTest"));
	if (!TestNotNull(TEXT("Created guided-event test world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_RegionEventDirector* Director = World->SpawnActor<AGP_RegionEventDirector>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned authoritative region event director"), Director))
	{
		World->DestroyWorld(false);
		return false;
	}

	Director->bUseDeterministicRandomSeed = true;
	Director->bEnableExplorationEvents = false;
	Director->MaxActiveExplorationEvents = 1;
	Director->SetAmbientExplorationSuppressed(true);
	Director->InitializeRegionEventDirector(15);
	TestTrue(TEXT("Suppression can be requested before initialization"), Director->IsAmbientExplorationSuppressed());
	TestFalse(
		TEXT("Suppressed initialization does not schedule ambient discoveries"),
		World->GetTimerManager().IsTimerActive(Director->ExplorationEvaluationTimerHandle));

	TestNull(
		TEXT("Unknown guided ids fail closed"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("missing_event"), 2, FVector::ZeroVector));
	TestNull(
		TEXT("Out-of-range guided regions fail closed"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("world_red_rift"), 15, FVector::ZeroVector));

	if (!Director->ExplorationEventPool.IsEmpty())
	{
		UGP_RegionEventData* DuplicateData = Director->ExplorationEventPool[0];
		Director->ExplorationEventPool.Add(DuplicateData);
		TestNull(
			TEXT("Duplicate guided ids fail closed instead of selecting by array order"),
			Director->TryStartGuidedRegionEventAtLocation(DuplicateData->EventId, 2, FVector::ZeroVector));
		Director->ExplorationEventPool.RemoveAt(Director->ExplorationEventPool.Num() - 1);
	}

	const FVector RequestedLocation(120.0f, -340.0f, 80.0f);
	AGP_RegionEventActor* RedRift = Director->TryStartGuidedRegionEventAtLocation(
		TEXT("world_red_rift"),
		2,
		RequestedLocation,
		/*DormantWaitTimeoutSeconds=*/-1.0f);
	TestNotNull(TEXT("Explicit guided start works while ambient exploration is disabled"), RedRift);
	if (RedRift)
	{
		TestTrue(TEXT("Guided production id resolves to the Red Rift actor"), RedRift->IsA<AGP_RedRiftRegionEventActor>());
		TestNotNull(TEXT("Guided actor keeps its production data"), RedRift->GetEventData());
		if (RedRift->GetEventData())
		{
			TestEqual(TEXT("Guided actor keeps the specifically requested EventId"), RedRift->GetEventData()->EventId, FName(TEXT("world_red_rift")));
		}
		TestEqual(TEXT("Guided event keeps the requested region"), RedRift->GetRegionId(), 2);
		TestTrue(
			TEXT("Director spawn offset is applied exactly once"),
			RedRift->GetActorLocation().Equals(RequestedLocation + Director->SpawnOffset));
		TestTrue(TEXT("Guided event uses exploration ownership"), Director->IsExplorationEvent(RedRift));
		TestEqual(
			TEXT("Guided event remains dormant until the party approaches"),
			RedRift->GetRuntimeState(),
			EGPRegionEventRuntimeState::Dormant);
		TestTrue(TEXT("Guided event is waiting for approach"), RedRift->IsWaitingForPlayerApproach());
		TestEqual(TEXT("Guided event requires the configured party quorum"), RedRift->GetRequiredApproachPlayerCount(), 2);
		TestEqual(TEXT("Negative guided timeout remains unlimited"), RedRift->GetConfiguredDormantWaitTimeout(), -1.0f);
		TestFalse(
			TEXT("Unlimited guided discovery owns no dormant-expiration timer"),
			World->GetTimerManager().IsTimerActive(RedRift->DormantWaitTimerHandle));
	}

	TestNull(
		TEXT("A second guided event respects the exploration concurrency cap"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("world_shrine_ruins"), 3, FVector::ZeroVector));
	if (RedRift)
	{
		RedRift->CompleteRegionEvent();
	}
	AGP_RegionEventActor* Shrine = Director->TryStartGuidedRegionEventAtLocation(
		TEXT("world_shrine_ruins"),
		3,
		FVector(500.0f, 0.0f, 0.0f));
	TestNotNull(TEXT("Completing a guided event reopens exploration capacity"), Shrine);
	if (Shrine)
	{
		TestFalse(TEXT("One of three players cannot satisfy a guided quorum"), Shrine->HasApproachPlayerQuorum(1, 3));
		TestTrue(TEXT("Two of three players satisfy a guided quorum"), Shrine->HasApproachPlayerQuorum(2, 3));
		TestTrue(TEXT("A one-player local run clamps the quorum"), Shrine->HasApproachPlayerQuorum(1, 1));
	}

	Director->bEnableExplorationEvents = true;
	Director->ObservedRegionId = 4;
	Director->ObservedRegionSinceSeconds = 99.0;
	Director->SetAmbientExplorationSuppressed(false);
	TestEqual(TEXT("Unsuppress resets the observed region"), Director->ObservedRegionId, INDEX_NONE);
	TestEqual(TEXT("Unsuppress resets dwell time"), Director->ObservedRegionSinceSeconds, 0.0);
	TestTrue(
		TEXT("Unsuppress restarts the ambient evaluation timer"),
		World->GetTimerManager().IsTimerActive(Director->ExplorationEvaluationTimerHandle));
	Director->SetAmbientExplorationSuppressed(true);
	TestFalse(
		TEXT("Resuppress clears the ambient evaluation timer"),
		World->GetTimerManager().IsTimerActive(Director->ExplorationEvaluationTimerHandle));

	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
