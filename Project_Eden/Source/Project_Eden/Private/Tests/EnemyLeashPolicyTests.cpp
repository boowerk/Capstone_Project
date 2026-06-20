#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Services/EnemyLeashPolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyLeashPolicyTest,
	"ProjectEden.AI.Enemy.LeashPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyLeashPolicyTest::RunTest(const FString& Parameters)
{
	FEnemyLeashObservation Observation;
	Observation.DistanceFromHome = 2500.0f;
	Observation.MaxChaseDistanceFromHome = 2200.0f;
	Observation.ReturnHomeAcceptanceRadius = 150.0f;

	// This is the reported regression: crossing the leash must not discard a player who is still visibly engaged.
	Observation.bHasActiveTarget = true;
	TestFalse(TEXT("Visible combat target prevents an abrupt return home"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.bHasActiveTarget = false;
	TestTrue(TEXT("Losing the target outside the leash starts return home"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.bCurrentlyReturningHome = true;
	Observation.bHasActiveTarget = true;
	TestFalse(TEXT("Seeing the player again interrupts return home"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.bHasActiveTarget = false;
	TestTrue(TEXT("Return home remains active while still outside acceptance"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.DistanceFromHome = 100.0f;
	TestFalse(TEXT("Reaching home releases the leash"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	return true;
}

#endif
