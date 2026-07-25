#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/EnemyAttackCadencePolicy.h"
#include "Characters/GP_FlyingEnemyCharacter.h"
#include "Characters/GP_MeleeEnemyCharacter.h"
#include "Characters/GP_RangedEnemyCharacter.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackCadencePolicyTest,
	"ProjectEden.AI.Enemy.AttackCadence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackCadencePolicyTest::RunTest(const FString& Parameters)
{
	const FVector2D ReversedRange = EnemyAttackCadencePolicy::SanitizeDelayRange(2.0f, 1.0f);
	TestEqual(TEXT("A reversed maximum is raised to the minimum"), ReversedRange, FVector2D(2.0f, 2.0f));

	const FVector2D OversizedRange = EnemyAttackCadencePolicy::SanitizeDelayRange(-1.0f, 4.0f);
	TestEqual(TEXT("Cadence remains below the former three-second wait"), OversizedRange, FVector2D(0.0f, 2.95f));

	FRandomStream RandomStream(7123);
	for (int32 RollIndex = 0; RollIndex < 32; ++RollIndex)
	{
		const float Delay = EnemyAttackCadencePolicy::RollDelay(FVector2D(0.8f, 1.3f), RandomStream);
		TestTrue(TEXT("Rolled delay remains in the authored range"), Delay >= 0.8f && Delay <= 1.3f);
	}

	const FGPEnemyAttackCadenceSettings& MeleeSettings = GetDefault<AGP_MeleeEnemyCharacter>()->GetAttackCadenceSettings();
	const FGPEnemyAttackCadenceSettings& RangedSettings = GetDefault<AGP_RangedEnemyCharacter>()->GetAttackCadenceSettings();
	const FGPEnemyAttackCadenceSettings& FlyingSettings = GetDefault<AGP_FlyingEnemyCharacter>()->GetAttackCadenceSettings();
	TestTrue(TEXT("Melee attacks recover sooner than ranged attacks"), MeleeSettings.NextAttackDelayMaxSeconds < RangedSettings.NextAttackDelayMaxSeconds);
	TestTrue(TEXT("Flying cadence differs from grounded ranged cadence"), !FMath::IsNearlyEqual(FlyingSettings.NextAttackDelayMinSeconds, RangedSettings.NextAttackDelayMinSeconds));
	TestTrue(TEXT("Every basic archetype stays under three seconds"),
		MeleeSettings.NextAttackDelayMaxSeconds < 3.0f
		&& RangedSettings.NextAttackDelayMaxSeconds < 3.0f
		&& FlyingSettings.NextAttackDelayMaxSeconds < 3.0f);

	return true;
}

#endif
