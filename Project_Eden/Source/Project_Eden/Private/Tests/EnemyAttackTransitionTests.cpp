#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/EnemyAttackTransitionPolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackTransitionPolicyTest,
	"ProjectEden.AI.Enemy.AttackTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackTransitionPolicyTest::RunTest(const FString& Parameters)
{
	// 진입 범위는 엄격하게, 이미 교전한 적의 이탈 범위만 100cm 넓어진다.
	TestTrue(TEXT("An enemy enters at the authored outer edge"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(925.0f, 575.0f, 925.0f, true, false, 100.0f));
	TestFalse(TEXT("An enemy cannot enter beyond the authored outer edge"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(926.0f, 575.0f, 925.0f, true, false, 100.0f));
	TestTrue(TEXT("A latched enemy remains engaged inside the wider exit edge"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(1000.0f, 575.0f, 925.0f, true, true, 100.0f));
	TestFalse(TEXT("A latched enemy exits after crossing the hysteresis edge"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(1026.0f, 575.0f, 925.0f, true, true, 100.0f));

	TestTrue(TEXT("A ranged enemy retains its band slightly inside the minimum"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(500.0f, 575.0f, 925.0f, false, true, 100.0f));
	TestFalse(TEXT("A ranged enemy repositions after crossing the inner exit edge"),
		EnemyAttackTransitionPolicy::IsInsideAttackBand(474.0f, 575.0f, 925.0f, false, true, 100.0f));

	FEnemyAttackTransitionObservation Observation;
	Observation.bHasLineOfSight = true;
	Observation.bInsideAttackBand = true;
	Observation.bAttackCadenceReady = true;
	TestTrue(TEXT("A ready enemy chooses Attack"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::Attack);

	Observation.bAttackCadenceReady = false;
	TestTrue(TEXT("Cadence recovery inside range chooses CombatHold instead of Chase"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::CombatHold);

	Observation.bHasLineOfSight = false;
	Observation.bInsideAttackBand = false;
	Observation.bActionCommitted = true;
	TestTrue(TEXT("A committed action remains Attack through transient observations"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::Attack);

	Observation.bShouldRetreat = true;
	TestTrue(TEXT("Retreat still interrupts a committed action"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::None);

	return true;
}

#endif
