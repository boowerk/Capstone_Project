#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/World.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Player/GP_PlayerController.h"
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

	// Guided-only orchestration owns a fixed production pool and one active scripted beat at a time.
	TestEqual(TEXT("Guided director loads the three fixed graduation-demo definitions"), Director->GetGuidedEventPoolCount(), 3);
	Director->MaxActiveEvents = 1;
	Director->InitializeRegionEventDirector(15);

	TestNull(
		TEXT("Unknown guided ids fail closed"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("missing_event"), 2, FVector::ZeroVector));
	TestNull(
		TEXT("Out-of-range guided regions fail closed"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("world_red_rift"), 15, FVector::ZeroVector));

	if (!Director->GuidedEventPool.IsEmpty())
	{
		UGP_RegionEventData* DuplicateData = Director->GuidedEventPool[0];
		Director->GuidedEventPool.Add(DuplicateData);
		TestNull(
			TEXT("Duplicate guided ids fail closed instead of selecting by array order"),
			Director->TryStartGuidedRegionEventAtLocation(DuplicateData->EventId, 2, FVector::ZeroVector));
		Director->GuidedEventPool.RemoveAt(Director->GuidedEventPool.Num() - 1);
	}

	const FVector RequestedLocation(120.0f, -340.0f, 80.0f);
	AGP_RegionEventActor* RedRift = Director->TryStartGuidedRegionEventAtLocation(
		TEXT("world_red_rift"),
		2,
		RequestedLocation,
		/*DormantWaitTimeoutSeconds=*/-1.0f);
	TestNotNull(TEXT("Explicit guided EventId starts its scripted encounter"), RedRift);
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
		TestEqual(TEXT("Director tracks the active guided encounter"), Director->GetActiveEventCount(), 1);
	}

	TestNull(
		TEXT("A second guided event respects MaxActiveEvents"),
		Director->TryStartGuidedRegionEventAtLocation(TEXT("world_shrine_ruins"), 3, FVector::ZeroVector));
	if (RedRift)
	{
		RedRift->CompleteRegionEvent();
	}
	TestEqual(TEXT("Completing a guided event releases its active slot"), Director->GetActiveEventCount(), 0);
	AGP_RegionEventActor* Shrine = Director->TryStartGuidedRegionEventAtLocation(
		TEXT("world_shrine_ruins"),
		3,
		FVector(500.0f, 0.0f, 0.0f));
	TestNotNull(TEXT("Completing a guided event reopens scripted capacity"), Shrine);
	if (Shrine)
	{
		// The graduation demo is a three-player game; the configured majority quorum is two possessed players.
		TestFalse(TEXT("One of three players cannot satisfy a guided quorum"), Shrine->HasApproachPlayerQuorum(1, 3));
		TestTrue(TEXT("Two of three players satisfy a guided quorum"), Shrine->HasApproachPlayerQuorum(2, 3));
		TestTrue(TEXT("A one-player local run clamps the quorum"), Shrine->HasApproachPlayerQuorum(1, 1));

		AGP_ShrineRuinsRegionEventActor* GuidedShrine = Cast<AGP_ShrineRuinsRegionEventActor>(Shrine);
		TestNotNull(TEXT("Guided shrine uses its specialized reward actor"), GuidedShrine);
		if (GuidedShrine)
		{
			TestTrue(TEXT("Director enables full-party shrine rewards"), GuidedShrine->IsGuidedPartyRewardEnabled());
			TArray<AGP_PlayerController*> PartyControllers;
			for (int32 PlayerIndex = 0; PlayerIndex < 3; ++PlayerIndex)
			{
				PartyControllers.Add(World->SpawnActor<AGP_PlayerController>(
					FVector(PlayerIndex * 100.0f, 0.0f, 0.0f),
					FRotator::ZeroRotator,
					SpawnParameters));
			}
			GuidedShrine->ConfigureApproachActivation(false, 0.0f, -1.0f);
			GuidedShrine->ActivateRegionEvent();
			GuidedShrine->RewardPartyControllers(PartyControllers);
			TestEqual(TEXT("Guided shrine dispatches rewards to all three controllers"), GuidedShrine->GetRewardedControllerCount(), 3);
			TestEqual(
				TEXT("Guided shrine completes after the full dispatch"),
				GuidedShrine->GetRuntimeState(),
				EGPRegionEventRuntimeState::Completed);
			TestEqual(TEXT("Completed shrine releases the guided lifecycle slot"), Director->GetActiveEventCount(), 0);
		}
	}

	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
