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
	Observation.ReengageDistanceFromHome = 1650.0f;
	Observation.ReturnHomeAcceptanceRadius = 150.0f;

	// Crossing the authored anchor boundary must start a real return even if the chased player remains visible.
	Observation.bHasVisibleTarget = true;
	TestTrue(TEXT("Crossing the outer leash starts return home"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.bCurrentlyReturningHome = true;
	TestTrue(TEXT("Visible target outside the inner boundary cannot cause return oscillation"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.DistanceFromHome = 1500.0f;
	TestFalse(TEXT("Visible target inside the inner boundary interrupts return home"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.bHasVisibleTarget = false;
	TestTrue(TEXT("Return home remains active while still outside acceptance"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	Observation.DistanceFromHome = 100.0f;
	TestFalse(TEXT("Reaching home releases the leash"), EnemyLeashPolicy::ShouldReturnHome(Observation));

	return true;
}

#endif
