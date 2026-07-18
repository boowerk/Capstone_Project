#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/BossAttackTransitionPolicy.h"
#include "AI/Services/BTS_UpdateEnemyTactics.h"
#include "AI/Services/BTS_UpdateMatadorTactics.h"
#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"
#include "AbilitySystemComponent.h"
#include "Actors/GP_BullChargeActor.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BTTaskNode.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Engine/World.h"
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
	TestFalse(
		TEXT("A live bull remains committed after the generic eight-second timeout"),
		BossAttackTransitionPolicy::ShouldCompleteExternalAction(true, 9.0f, 20.0f));
	TestTrue(
		TEXT("Bull signal release completes the external action immediately"),
		BossAttackTransitionPolicy::ShouldCompleteExternalAction(false, 9.0f, 20.0f));
	TestTrue(
		TEXT("A stuck bull is released only at its external action cap"),
		BossAttackTransitionPolicy::ShouldCompleteExternalAction(true, 20.0f, 20.0f));

	const UBTT_ExecuteEnemyAttack* AttackTaskDefaults = GetDefault<UBTT_ExecuteEnemyAttack>();
	TestTrue(
		TEXT("BT stuck cap remains later than the bull actor absolute cap"),
		IsValid(AttackTaskDefaults) && AttackTaskDefaults->GetExternalBossActionTimeoutSeconds() >= 20.0f);

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMatadorBullLifecycleCleanupTest,
	"ProjectEden.AI.Boss.Matador.BullLifecycleCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMatadorBullLifecycleCleanupTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created Matador bull lifecycle world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_MatadorMageBossCharacter* Matador = TestWorld->SpawnActor<AGP_MatadorMageBossCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	UGP_MatadorBossStateComponent* State = IsValid(Matador) ? Matador->GetMatadorStateComponent() : nullptr;
	UAbilitySystemComponent* ASC = IsValid(Matador) ? Matador->GetAbilitySystemComponent() : nullptr;
	if (!TestNotNull(TEXT("Spawned Matador"), Matador)
		|| !TestNotNull(TEXT("Matador state exists"), State)
		|| !TestNotNull(TEXT("Matador ASC exists"), ASC))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	auto SpawnRegisteredBull = [&]() -> AGP_BullChargeActor*
	{
		AGP_BullChargeActor* Bull = TestWorld->SpawnActor<AGP_BullChargeActor>(
			FVector(500.0f, 0.0f, 0.0f),
			FRotator::ZeroRotator,
			SpawnParameters);
		if (IsValid(Bull))
		{
			// 테스트에서는 Matador 자체를 안전한 placeholder target으로 사용하고 lifecycle 등록만 검증한다.
			Bull->InitializeBullCharge(Matador, Matador, nullptr, State, FVector::ForwardVector);
		}
		return Bull;
	};

	AGP_BullChargeActor* ExternallyDestroyedBull = SpawnRegisteredBull();
	TestNotNull(TEXT("Spawned externally destroyed bull"), ExternallyDestroyedBull);
	TestTrue(TEXT("State registers the active bull"), IsValid(State) && State->GetActiveBullActor() == ExternallyDestroyedBull);
	TestTrue(
		TEXT("Active bull applies the Matador melee tag"),
		IsValid(ASC) && ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive));

	if (IsValid(ExternallyDestroyedBull))
	{
		ExternallyDestroyedBull->Destroy();
	}
	TestNull(TEXT("External Destroy clears the active bull pointer"), State->GetActiveBullActor());
	TestFalse(
		TEXT("External Destroy clears the Matador melee tag"),
		ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive));

	AGP_BullChargeActor* StuckBull = SpawnRegisteredBull();
	TestNotNull(TEXT("Spawned stuck-cap bull"), StuckBull);
	if (IsValid(StuckBull))
	{
		// Tick the authoritative absolute cap directly; it must work even before the telegraph timer starts the charge.
		StuckBull->Tick(18.1f);
	}
	TestNull(TEXT("Absolute cap clears a stuck bull pointer"), State->GetActiveBullActor());
	TestFalse(
		TEXT("Absolute cap clears a stuck bull tag"),
		ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive));

	AGP_BullChargeActor* ForceEndedBull = SpawnRegisteredBull();
	TestNotNull(TEXT("Spawned force-ended bull"), ForceEndedBull);
	Matador->ForceEndBullPattern();
	TestNull(TEXT("BT fallback cleanup clears the active bull pointer"), State->GetActiveBullActor());
	TestFalse(
		TEXT("BT fallback cleanup clears the active bull tag"),
		ASC->HasMatchingGameplayTag(GPTags::State::Status::Enemy::MatadorMeleeActive));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
