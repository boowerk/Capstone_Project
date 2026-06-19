#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Tasks/BossAttackPatternSelector.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

namespace BossAttackPatternSelectorTests
{
	FGPBossAttackPatternContext MakeBaseContext()
	{
		FGPBossAttackPatternContext Context;
		Context.DefaultAttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
		Context.DistanceToTarget = 600.0f;
		Context.Aggression = 0.5f;
		Context.PreferredRange = 600.0f;
		Context.HealthRatio = 1.0f;
		Context.BossPhase = 2;
		Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Patrol);
		Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);
		return Context;
	}

	bool ExpectTopPattern(FAutomationTestBase& Test, const TCHAR* CaseName, const FGPBossAttackPatternContext& Context, const FGameplayTag& ExpectedTag)
	{
		const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
		const FGPBossAttackPatternCandidate* SelectedCandidate = FGPBossAttackPatternSelector::SelectBestCandidate(Candidates);
		const FString CandidateText = FGPBossAttackPatternSelector::DescribeCandidates(Candidates);

		if (!Test.TestTrue(FString::Printf(TEXT("%s produced candidates: %s"), CaseName, *CandidateText), SelectedCandidate != nullptr))
		{
			return false;
		}

		// Include the full candidate list in the assertion so a future score regression explains itself.
		return Test.TestTrue(
			FString::Printf(
				TEXT("%s expected %s but selected %s. Candidates: %s"),
				CaseName,
				*ExpectedTag.ToString(),
				*SelectedCandidate->AbilityTag.ToString(),
				*CandidateText),
			SelectedCandidate->AbilityTag.MatchesTagExact(ExpectedTag));
	}

	bool ExpectNoDamagePattern(FAutomationTestBase& Test, const TCHAR* CaseName, const FGPBossAttackPatternContext& Context)
	{
		const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(Context);
		const FGPBossAttackPatternCandidate* SelectedCandidate = FGPBossAttackPatternSelector::SelectBestCandidate(Candidates);

		// Out-of-reach damage patterns should fail closed so the BT can chase instead of playing whiffed attacks.
		return Test.TestTrue(
			FString::Printf(TEXT("%s expected no damage candidates. Candidates: %s"), CaseName, *FGPBossAttackPatternSelector::DescribeCandidates(Candidates)),
			SelectedCandidate == nullptr);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossAttackPatternSelectorScoresTest,
	"ProjectEden.AI.Boss.PatternSelector.ScoreCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossAttackPatternSelectorScoresTest::RunTest(const FString& Parameters)
{
	using namespace BossAttackPatternSelectorTests;

	FGPBossAttackPatternContext Context = MakeBaseContext();
	Context.DistanceToTarget = 240.0f;
	ExpectTopPattern(*this, TEXT("No boss pattern window falls back to basic"), Context, GPTags::Ability::Enemy::Attack_Melee);

	Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.Aggression = 0.95f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst);
	ExpectTopPattern(*this, TEXT("Pressure evaluation prefers sweep"), Context, GPTags::Ability::Enemy::Attack_BossSweep);

	Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossHeavyAttack = true;
	Context.DistanceToTarget = 160.0f;
	Context.Aggression = 0.15f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest);
	ExpectTopPattern(*this, TEXT("Close defensive evaluation escapes sweep spam with basic"), Context, GPTags::Ability::Enemy::Attack_Melee);

	Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossAreaAttack = true;
	Context.DistanceToTarget = 820.0f;
	Context.Aggression = 0.2f;
	Context.PreferredRange = 120.0f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	ExpectTopPattern(*this, TEXT("Far hold evaluation prefers area"), Context, GPTags::Ability::Enemy::Attack_BossArea);

	Context = MakeBaseContext();
	Context.bSuppressGenericBossAttacks = true;
	Context.bCanUseBossHeavyAttack = true;
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossAreaAttack = true;
	Context.bCanSummonAdds = true;
	Context.DistanceToTarget = 1300.0f;
	ExpectNoDamagePattern(*this, TEXT("Matador suppresses common boss melee and area patterns outside special range"), Context);

	Context = MakeBaseContext();
	Context.bSuppressGenericBossAttacks = true;
	Context.DistanceToTarget = 520.0f;
	ExpectTopPattern(*this, TEXT("Matador close range uses cape gust"), Context, GPTags::Ability::Boss::Matador::CapeGust);

	Context = MakeBaseContext();
	Context.bSuppressGenericBossAttacks = true;
	Context.DistanceToTarget = 900.0f;
	ExpectTopPattern(*this, TEXT("Matador mid range uses rapier thrust"), Context, GPTags::Ability::Boss::Matador::RapierThrust);

	Context = MakeBaseContext();
	Context.bSuppressGenericBossAttacks = true;
	Context.bCanUseBullPattern = true;
	Context.DistanceToTarget = 900.0f;
	ExpectTopPattern(*this, TEXT("Matador bull pattern outranks melee specials"), Context, GPTags::Ability::Enemy::Utility_MatadorBullPattern);

	Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossAreaAttack = true;
	Context.bCanUseBossHeavyAttack = true;
	Context.DistanceToTarget = 1000.0f;
	ExpectNoDamagePattern(*this, TEXT("Out-of-reach damage patterns are filtered"), Context);

	Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossAreaAttack = true;
	Context.bCanSummonAdds = true;
	Context.DistanceToTarget = 1300.0f;
	Context.Aggression = 0.2f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	ExpectTopPattern(*this, TEXT("Retreat evaluation opens summon pressure"), Context, GPTags::Ability::Enemy::Utility_BossSummon);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSansGroundHandsPatternSelectorTest,
	"ProjectEden.AI.Boss.PatternSelector.SansGroundHands",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSansGroundHandsPatternSelectorTest::RunTest(const FString& Parameters)
{
	using namespace BossAttackPatternSelectorTests;

	FGPBossAttackPatternContext Context = MakeBaseContext();
	Context.bCanUseBossGroundHands = true;
	Context.DistanceToTarget = 620.0f;
	return ExpectTopPattern(
		*this,
		TEXT("Sans generic boss evaluation selects ground hands"),
		Context,
		GPTags::Ability::Enemy::Attack_BossGroundHands);
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphPatternSelectorTest,
	"ProjectEden.AI.Boss.PatternSelector.CrystalSeraph",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphPatternSelectorTest::RunTest(const FString& Parameters)
{
	using namespace BossAttackPatternSelectorTests;

	FGPBossAttackPatternContext Context = MakeBaseContext();
	Context.bIsCrystalSeraph = true;
	Context.bShouldTeleport = true;
	Context.DistanceToTarget = 2300.0f;
	ExpectTopPattern(*this, TEXT("Crystal Seraph teleports before distant attacks"), Context, GPTags::Ability::Boss::CrystalSeraph::Teleport);

	Context = MakeBaseContext();
	Context.bIsCrystalSeraph = true;
	Context.DistanceToTarget = 1100.0f;
	Context.PreferredAirRange = 1100.0f;
	ExpectTopPattern(*this, TEXT("Crystal Seraph uses shard as its normal range fallback"), Context, GPTags::Ability::Boss::CrystalSeraph::Basic);

	// Crystal mechanics must outrank the shard fallback whenever their service-owned readiness flags are open.
	Context.bCanUsePrismPattern = true;
	ExpectTopPattern(*this, TEXT("Crystal Seraph creates a ready prism before using shards"), Context, GPTags::Ability::Boss::CrystalSeraph::Prism);

	Context.bCanUsePrismPattern = false;
	Context.bCrystalPrismActive = true;
	Context.bCanUseLaserPattern = true;
	ExpectTopPattern(*this, TEXT("Crystal Seraph fires its laser while a prism is active"), Context, GPTags::Ability::Boss::CrystalSeraph::Laser);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossAttackPatternSelectorLiveEvaluationTest,
	"ProjectEden.AI.Boss.PatternSelector.LiveEvaluationChangesPattern",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossAttackPatternSelectorLiveEvaluationTest::RunTest(const FString& Parameters)
{
	using namespace BossAttackPatternSelectorTests;

	FGPBossAttackPatternContext Context = MakeBaseContext();
	Context.bCanUseBossSweepAttack = true;
	Context.bCanUseBossAreaAttack = true;
	Context.bCanSummonAdds = true;
	Context.bCanUseBossHeavyAttack = true;

	Context.DistanceToTarget = 180.0f;
	Context.Aggression = 0.2f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest);
	ExpectTopPattern(*this, TEXT("Live eval 1: hold/close"), Context, GPTags::Ability::Enemy::Attack_Melee);

	Context.DistanceToTarget = 600.0f;
	Context.Aggression = 0.95f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst);
	ExpectTopPattern(*this, TEXT("Live eval 2: pressure/preferred range"), Context, GPTags::Ability::Enemy::Attack_BossSweep);

	Context.bCanUseBossHeavyAttack = false;
	Context.DistanceToTarget = 1350.0f;
	Context.Aggression = 0.15f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);
	ExpectTopPattern(*this, TEXT("Live eval 3: retreat/far"), Context, GPTags::Ability::Enemy::Utility_BossSummon);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBossAttackPatternSelectorRuntimeTestCycleTest,
	"ProjectEden.AI.Boss.PatternSelector.RuntimeTestCycleCases",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBossAttackPatternSelectorRuntimeTestCycleTest::RunTest(const FString& Parameters)
{
	using namespace BossAttackPatternSelectorTests;

	// Mirrors the controller's runtime debug cycle so pattern visibility regressions fail in automation.
	FGPBossAttackPatternContext Context = MakeBaseContext();
	Context.BossPhase = 1;
	Context.bCanUseBossHeavyAttack = true;
	Context.DistanceToTarget = 220.0f;
	Context.Aggression = 0.2f;
	Context.PreferredRange = 650.0f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest);
	ExpectTopPattern(*this, TEXT("Runtime cycle sample 0: basic"), Context, GPTags::Ability::Enemy::Attack_Melee);

	Context = MakeBaseContext();
	Context.BossPhase = 1;
	Context.bCanUseBossSweepAttack = true;
	Context.DistanceToTarget = 600.0f;
	Context.Aggression = 0.95f;
	Context.PreferredRange = 600.0f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst);
	ExpectTopPattern(*this, TEXT("Runtime cycle sample 1: sweep"), Context, GPTags::Ability::Enemy::Attack_BossSweep);

	Context = MakeBaseContext();
	Context.BossPhase = 1;
	Context.bCanUseBossAreaAttack = true;
	Context.DistanceToTarget = 600.0f;
	Context.Aggression = 0.55f;
	Context.PreferredRange = 120.0f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);
	ExpectTopPattern(*this, TEXT("Runtime cycle sample 2: area"), Context, GPTags::Ability::Enemy::Attack_BossArea);

	Context = MakeBaseContext();
	Context.BossPhase = 1;
	Context.bCanSummonAdds = true;
	Context.DistanceToTarget = 600.0f;
	Context.Aggression = 0.15f;
	Context.PreferredRange = 120.0f;
	Context.EnemyMode = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat);
	Context.FocusTargetRule = FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);
	ExpectTopPattern(*this, TEXT("Runtime cycle sample 3: summon"), Context, GPTags::Ability::Enemy::Utility_BossSummon);

	return true;
}

#endif
