#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Engine/World.h"
#include "Game/Demo/GP_DemoRunDirector.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPDemoRunGoldenPathContractTest,
	"ProjectEden.Game.DemoFlow.GoldenPathContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPDemoRunGoldenPathContractTest::RunTest(const FString& Parameters)
{
	// Keep the authored experience stable while allowing the route policy to vary only its locations.
	TestEqual(
		TEXT("Opening stage uses the Red Rift production id"),
		AGP_DemoRunDirector::GetEventIdForStage(EGPDemoRunStage::OpeningRedRift),
		FName(TEXT("world_red_rift")));
	TestEqual(
		TEXT("Pressure stage uses the Structure Defense production id"),
		AGP_DemoRunDirector::GetEventIdForStage(EGPDemoRunStage::PressureDefense),
		FName(TEXT("world_structure_defense")));
	TestEqual(
		TEXT("Reward stage uses the Shrine production id"),
		AGP_DemoRunDirector::GetEventIdForStage(EGPDemoRunStage::InnerShrine),
		FName(TEXT("world_shrine_ruins")));

	TestEqual(
		TEXT("Opening stage uses the first inward objective"),
		AGP_DemoRunDirector::GetRouteRoleForStage(EGPDemoRunStage::OpeningRedRift),
		EGPDemoRouteRole::OpeningObjective);
	TestEqual(
		TEXT("Pressure stage uses the second inward objective"),
		AGP_DemoRunDirector::GetRouteRoleForStage(EGPDemoRunStage::PressureDefense),
		EGPDemoRouteRole::PressureObjective);
	TestEqual(
		TEXT("Shrine stage uses the inner reward point"),
		AGP_DemoRunDirector::GetRouteRoleForStage(EGPDemoRunStage::InnerShrine),
		EGPDemoRouteRole::InnerReward);
	TestEqual(
		TEXT("Rally stage uses the center boss point"),
		AGP_DemoRunDirector::GetRouteRoleForStage(EGPDemoRunStage::CenterRally),
		EGPDemoRouteRole::CenterBoss);

	TestEqual(
		TEXT("Red Rift advances to defense"),
		AGP_DemoRunDirector::GetNextStageAfterEvent(EGPDemoRunStage::OpeningRedRift),
		EGPDemoRunStage::PressureDefense);
	TestEqual(
		TEXT("Defense advances to shrine"),
		AGP_DemoRunDirector::GetNextStageAfterEvent(EGPDemoRunStage::PressureDefense),
		EGPDemoRunStage::InnerShrine);
	TestEqual(
		TEXT("Shrine advances to reward presentation"),
		AGP_DemoRunDirector::GetNextStageAfterEvent(EGPDemoRunStage::InnerShrine),
		EGPDemoRunStage::RewardGrace);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("DemoRunDirectorContractTest"));
	if (!TestNotNull(TEXT("Created demo director contract world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_DemoRunDirector* RunDirector = World->SpawnActor<AGP_DemoRunDirector>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_RegionEventDirector* EventDirector = World->SpawnActor<AGP_RegionEventDirector>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_RegionEventActor* ExpectedEvent = World->SpawnActor<AGP_RegionEventActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_RegionEventActor* UnrelatedEvent = World->SpawnActor<AGP_RegionEventActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	const bool bSpawnedContractActors = TestNotNull(TEXT("Spawned demo run director"), RunDirector)
		&& TestNotNull(TEXT("Spawned region event director"), EventDirector)
		&& TestNotNull(TEXT("Spawned expected guided event"), ExpectedEvent)
		&& TestNotNull(TEXT("Spawned unrelated guided event"), UnrelatedEvent);
	if (bSpawnedContractActors)
	{
		UClass* ProductionBossClass = LoadClass<AGP_DarkArmorKnightBossCharacter>(
			nullptr,
			TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight.BP_DarkArmorKnight_C"));
		TestNotNull(TEXT("Production Dark Armor Knight Blueprint loads"), ProductionBossClass);
		TestTrue(
			TEXT("Production Dark Armor Knight class resolves to the expected native boss"),
			*RunDirector->DarkArmorKnightClass
			&& RunDirector->DarkArmorKnightClass->IsChildOf(AGP_DarkArmorKnightBossCharacter::StaticClass()));
		TestTrue(
			TEXT("Demo flow configures the production Dark Armor Knight Blueprint"),
			RunDirector->DarkArmorKnightClass.Get() == ProductionBossClass);

		// Only the exact actor owned by the current guided beat may advance the run.
		RunDirector->RegionEventDirector = EventDirector;
		RunDirector->CurrentEvent = ExpectedEvent;
		RunDirector->CurrentStage = EGPDemoRunStage::OpeningRedRift;
		RunDirector->HandleGuidedEventEnded(EventDirector, UnrelatedEvent);
		TestEqual(
			TEXT("An unrelated event cannot advance the current stage"),
			RunDirector->CurrentStage,
			EGPDemoRunStage::OpeningRedRift);
		TestTrue(
			TEXT("An unrelated event cannot replace the tracked objective"),
			RunDirector->CurrentEvent == ExpectedEvent);

		RunDirector->bInitialized = true;
		RunDirector->StopDemoRun();
		TestTrue(TEXT("Stopping the demo flow enters its terminal guard"), RunDirector->bFinishingRun);
		TestEqual(
			TEXT("Stopping the demo flow publishes a terminal stage"),
			RunDirector->CurrentStage,
			EGPDemoRunStage::Completed);
		TestNull(TEXT("Stopping the demo flow releases the current event"), RunDirector->CurrentEvent.Get());
		TestEqual(
			TEXT("Stopping the demo flow retires the current event"),
			ExpectedEvent->GetRuntimeState(),
			EGPRegionEventRuntimeState::Expired);
	}

	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
