#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/BossAttackTransitionPolicy.h"
#include "AI/Services/BTS_UpdateEnemyTactics.h"
#include "AI/Services/BTS_UpdateMatadorTactics.h"
#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

namespace BossAttackTransitionTests
{
	bool ContainsTaskClass(const UBTCompositeNode* CompositeNode, const UClass* TaskClass)
	{
		if (!IsValid(CompositeNode) || !IsValid(TaskClass))
		{
			return false;
		}

		for (const FBTCompositeChild& Child : CompositeNode->Children)
		{
			if (IsValid(Child.ChildTask) && Child.ChildTask->IsA(TaskClass))
			{
				return true;
			}
			if (ContainsTaskClass(Child.ChildComposite, TaskClass))
			{
				return true;
			}
		}
		return false;
	}

	bool ContainsServiceClass(const UBTCompositeNode* CompositeNode, const UClass* ServiceClass)
	{
		if (!IsValid(CompositeNode) || !IsValid(ServiceClass))
		{
			return false;
		}

		for (const UBTService* Service : CompositeNode->Services)
		{
			if (IsValid(Service) && Service->IsA(ServiceClass))
			{
				return true;
			}
		}

		for (const FBTCompositeChild& Child : CompositeNode->Children)
		{
			if (IsValid(Child.ChildTask))
			{
				for (const UBTService* Service : Child.ChildTask->Services)
				{
					if (IsValid(Service) && Service->IsA(ServiceClass))
					{
						return true;
					}
				}
			}
			if (ContainsServiceClass(Child.ChildComposite, ServiceClass))
			{
				return true;
			}
		}
		return false;
	}
}

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

	// Ranged candidates must open the common Attack branch even when no melee
	// pattern can reach the current target.
	TestTrue(
		TEXT("Charge at 1200cm can request the Dark Knight attack branch"),
		BossAttackTransitionPolicy::CanRequestDarkKnightPattern(false, false, true, false, false));
	TestTrue(
		TEXT("Dark Wave at 2000cm can request the Dark Knight attack branch"),
		BossAttackTransitionPolicy::CanRequestDarkKnightPattern(false, false, false, true, false));
	TestTrue(
		TEXT("Ground Crack can independently request the Dark Knight attack branch"),
		BossAttackTransitionPolicy::CanRequestDarkKnightPattern(false, false, false, false, true));
	TestFalse(
		TEXT("Dark Knight chases when no reachable pattern is ready"),
		BossAttackTransitionPolicy::CanRequestDarkKnightPattern(false, false, false, false, false));

	const UBTS_UpdateMatadorTactics* MatadorPolicy = GetDefault<UBTS_UpdateMatadorTactics>();
	TestTrue(
		TEXT("Matador bull opens during its authored window without a boss-only Blackboard key"),
		IsValid(MatadorPolicy)
			&& MatadorPolicy->IsBullPatternReady(true, false, false, false, 1200.0f, false, 1.0f));
	TestFalse(
		TEXT("Matador bull stays closed outside its authored window"),
		IsValid(MatadorPolicy)
			&& MatadorPolicy->IsBullPatternReady(true, false, false, false, 1200.0f, true, 4.0f));
	TestFalse(
		TEXT("Matador bull cannot reopen while its external action is active"),
		IsValid(MatadorPolicy)
			&& MatadorPolicy->IsBullPatternReady(true, false, false, true, 1200.0f, true, 1.0f));

	UBehaviorTree* MatadorTree = LoadObject<UBehaviorTree>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/BT/Boss/BT_Boss_Matador.BT_Boss_Matador"));
	TestNotNull(TEXT("Production Matador behavior tree loads"), MatadorTree);
	if (IsValid(MatadorTree))
	{
		// Validate the real serialized graph rather than an unused dedicated node.
		TestTrue(
			TEXT("Production Matador tree executes the shared latent attack task"),
			BossAttackTransitionTests::ContainsTaskClass(
				MatadorTree->RootNode,
				UBTT_ExecuteEnemyAttack::StaticClass()));
		TestTrue(
			TEXT("Production Matador tree runs the common specialization service"),
			BossAttackTransitionTests::ContainsServiceClass(
				MatadorTree->RootNode,
				UBTS_UpdateEnemyTactics::StaticClass()));
	}
	return true;
}

#endif
