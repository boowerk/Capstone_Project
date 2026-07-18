#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/BossAttackTransitionPolicy.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossAttackTransitionTimingTest,
	"ProjectEden.AI.Boss.AttackTransitionTiming",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossAttackTransitionTimingTest::RunTest(const FString& Parameters)
{
	const float GroundCrackCommit = BossAttackTransitionPolicy::ResolvePostAbilityCommitSeconds(
		GPTags::Ability::Boss::DarkKnight::GroundCrack,
		0.0f);
	const float LaserCommit = BossAttackTransitionPolicy::ResolvePostAbilityCommitSeconds(
		GPTags::Ability::Boss::CrystalSeraph::Laser,
		0.0f);
	const float CrystalShardCommit = BossAttackTransitionPolicy::ResolvePostAbilityCommitSeconds(
		GPTags::Ability::Boss::CrystalSeraph::Basic,
		0.0f);

	// Regression thresholds mirror the delayed impact coordinators owned by the
	// boss actors without coupling the test to every exact tuning decimal.
	TestTrue(TEXT("Ground crack stays committed through wind-up and telegraph"), GroundCrackCommit >= 3.4f);
	TestTrue(TEXT("Crystal laser stays committed through its active beam"), LaserCommit >= 3.7f);
	TestTrue(TEXT("Crystal shard returns control promptly after launch"), CrystalShardCommit < 1.0f);
	TestEqual(
		TEXT("Unknown active abilities keep the task's configured fallback"),
		BossAttackTransitionPolicy::ResolvePostAbilityCommitSeconds(FGameplayTag(), 0.45f),
		0.45f);
	return true;
}

#endif
