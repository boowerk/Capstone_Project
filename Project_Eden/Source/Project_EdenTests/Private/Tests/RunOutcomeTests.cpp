#if WITH_DEV_AUTOMATION_TESTS

#include "Game/GP_RunProgressionPolicy.h"
#include "Game/GP_GameMode.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPartyDefeatPolicyTest,
	"ProjectEden.RunOutcome.PartyDefeatPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPartyDefeatPolicyTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("An empty dedicated server is not a defeated party"),
		GPRunProgressionPolicy::IsPartyDefeated(0, 0));
	TestFalse(
		TEXT("One eliminated player does not end a three-player run"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 1));
	TestFalse(
		TEXT("One survivor keeps the run alive"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 2));
	TestTrue(
		TEXT("All three eliminated players end the run"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 3));
	TestTrue(
		TEXT("Solo elimination ends a solo run"),
		GPRunProgressionPolicy::IsPartyDefeated(1, 1));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPFinishRunClearsPendingCallbacksTest,
	"ProjectEden.RunOutcome.FinishClearsPendingCallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPFinishRunClearsPendingCallbacksTest::RunTest(
	const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created run-outcome test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_GameMode* GameMode = TestWorld->SpawnActor<AGP_GameMode>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned gameplay GameMode"), GameMode))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	FTimerHandle PendingCallbackHandle;
	TestWorld->GetTimerManager().SetTimer(
		PendingCallbackHandle,
		GameMode,
		&AGP_GameMode::NotifyAllPlayersDead,
		60.0f,
		false);
	TestTrue(
		TEXT("Object-bound callback exists before the run finishes"),
		TestWorld->GetTimerManager().TimerExists(
			PendingCallbackHandle));

	GameMode->NotifyAllPlayersDead();
	TestFalse(
		TEXT("Finishing the run clears pre-existing GameMode callbacks"),
		TestWorld->GetTimerManager().TimerExists(
			PendingCallbackHandle));

	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
