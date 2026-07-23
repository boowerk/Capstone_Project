#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Combat/EnemyAttackTransitionPolicy.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace EnemyAttackTransitionTests
{
	const UBTTask_MoveTo* FindMovingTargetChaseTask(const UBTCompositeNode* CompositeNode)
	{
		if (!IsValid(CompositeNode))
		{
			return nullptr;
		}

		for (const FBTCompositeChild& Child : CompositeNode->Children)
		{
			if (const UBTTask_MoveTo* MoveToTask = Cast<UBTTask_MoveTo>(Child.ChildTask))
			{
				if (MoveToTask->GetSelectedBlackboardKey() == EnemyBlackboardKeys::TargetActor)
				{
					return MoveToTask;
				}
			}
			if (const UBTTask_MoveTo* NestedTask = FindMovingTargetChaseTask(Child.ChildComposite))
			{
				return NestedTask;
			}
		}
		return nullptr;
	}
}

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
	TestTrue(TEXT("Melee outside its close hold resumes pursuit during cadence recovery"),
		EnemyAttackTransitionPolicy::ShouldPursueDuringCadenceRecovery(
			EEnemyAttackTransitionIntent::CombatHold, true, false, 300.0f, 225.0f, 50.0f));
	TestFalse(TEXT("A holding melee does not restart pursuit inside the wider entry edge"),
		EnemyAttackTransitionPolicy::ShouldPursueDuringCadenceRecovery(
			EEnemyAttackTransitionIntent::CombatHold, true, false, 250.0f, 225.0f, 50.0f));
	TestTrue(TEXT("An active melee pursuit continues until the close hold edge"),
		EnemyAttackTransitionPolicy::ShouldPursueDuringCadenceRecovery(
			EEnemyAttackTransitionIntent::CombatHold, true, true, 250.0f, 225.0f, 50.0f));
	TestFalse(TEXT("Melee already inside pressure distance can hold between swings"),
		EnemyAttackTransitionPolicy::ShouldPursueDuringCadenceRecovery(
			EEnemyAttackTransitionIntent::CombatHold, true, true, 200.0f, 225.0f, 50.0f));
	TestFalse(TEXT("Ranged cadence recovery never collapses its authored spacing into chase"),
		EnemyAttackTransitionPolicy::ShouldPursueDuringCadenceRecovery(
			EEnemyAttackTransitionIntent::CombatHold, false, false, 900.0f, 225.0f, 50.0f));

	// A stale production PreferredRange can no longer expand regular melee beyond physical execution reach.
	TestTrue(TEXT("Regular melee maximum attack range is physically capped"), FMath::IsNearlyEqual(
		EnemyAttackTransitionPolicy::ResolveMaximumAttackRange(878.0f, true, 350.0f), 350.0f));
	TestTrue(TEXT("Ranged maximum attack range remains authored"), FMath::IsNearlyEqual(
		EnemyAttackTransitionPolicy::ResolveMaximumAttackRange(1175.0f, false, 350.0f), 1175.0f));

	Observation.bHasLineOfSight = false;
	Observation.bInsideAttackBand = false;
	Observation.bActionCommitted = true;
	TestTrue(TEXT("A committed action remains Attack through transient observations"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::Attack);

	Observation.bShouldRetreat = true;
	TestTrue(TEXT("A committed action finishes before a tactical retreat"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::Attack);

	Observation.bActionCommitted = false;
	TestTrue(TEXT("A non-committed enemy can choose Retreat"),
		EnemyAttackTransitionPolicy::ResolveIntent(Observation) == EEnemyAttackTransitionIntent::None);

	TestTrue(
		TEXT("Target loss and leash reevaluation preserve an accepted action"),
		EnemyAttackTransitionPolicy::ShouldPreserveCommittedAction(true, false));
	TestFalse(
		TEXT("Death or vulnerability interrupts an accepted action"),
		EnemyAttackTransitionPolicy::ShouldPreserveCommittedAction(true, true));
	TestFalse(
		TEXT("An action that never committed cannot defer a branch change"),
		EnemyAttackTransitionPolicy::ShouldPreserveCommittedAction(false, false));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackCommitTargetLifetimeTest,
	"ProjectEden.AI.Enemy.AttackCommitTargetLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackCommitTargetLifetimeTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created attack commit test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_EnemyCharacter* Enemy = TestWorld->SpawnActor<AGP_EnemyCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	ATargetPoint* Target = TestWorld->SpawnActor<ATargetPoint>(
		FVector(300.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned enemy"), Enemy)
		|| !TestNotNull(TEXT("Spawned committed target"), Target))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	Enemy->BeginBehaviorAttackCommit(Target, 8.0f);
	TestTrue(TEXT("Attack commit starts with a live target"), Enemy->IsBehaviorAttackCommitted());
	TestTrue(TEXT("Live committed target remains addressable"), Enemy->GetBehaviorAttackCommittedTarget() == Target);

	// Disconnect/destruction invalidates the target pointer before the running montage or timer action necessarily ends.
	TestWorld->DestroyActor(Target);
	TestTrue(TEXT("Destroyed target does not release the accepted action"), Enemy->IsBehaviorAttackCommitted());
	TestNull(TEXT("Destroyed target is never returned to attack code"), Enemy->GetBehaviorAttackCommittedTarget());

	Enemy->FinishBehaviorAttackCommit(0.0f);
	TestFalse(TEXT("Explicit completion releases the action"), Enemy->IsBehaviorAttackCommitted());
	TestWorld->DestroyWorld(false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackCancellationPolicyTest,
	"ProjectEden.AI.Enemy.AttackCancellation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackCancellationPolicyTest::RunTest(const FString& Parameters)
{
	// 정상 montage 종료만 누락 notify를 보정하고, interrupt/cancel은 새 피해를 만들지 않는다.
	TestTrue(
		TEXT("Normal completion applies a missing fallback hit"),
		UGP_EnemyAttack::ShouldApplyFallbackHit(false, false));
	TestFalse(
		TEXT("Cancelled completion never applies a missing hit"),
		UGP_EnemyAttack::ShouldApplyFallbackHit(false, true));
	TestFalse(
		TEXT("An already applied hit is never duplicated"),
		UGP_EnemyAttack::ShouldApplyFallbackHit(true, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackPresentationLifetimeTest,
	"ProjectEden.AI.Enemy.AttackPresentationLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackPresentationLifetimeTest::RunTest(const FString& Parameters)
{
	// ActionEnd closes gameplay timing only; the authored recovery tail and its
	// blend must remain alive until the montage actually completes.
	TestFalse(
		TEXT("ActionEnd does not terminate the attack montage"),
		UGP_EnemyAttack::ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::ActionEnd));
	TestFalse(
		TEXT("Blend-out start does not cut the remaining blend"),
		UGP_EnemyAttack::ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageBlendOut));
	TestTrue(
		TEXT("Natural montage completion releases the ability"),
		UGP_EnemyAttack::ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageCompleted));
	TestTrue(
		TEXT("An explicit montage interruption still cancels the ability"),
		UGP_EnemyAttack::ShouldFinishAttackForPresentationSignal(EEnemyAttackPresentationSignal::MontageInterrupted));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyAttackForwardStepPolicyTest,
	"ProjectEden.AI.Enemy.AttackForwardStep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyAttackForwardStepPolicyTest::RunTest(const FString& Parameters)
{
	// The lunge may proceed only while every independent movement and committed-target contract remains valid.
	TestTrue(TEXT("A live distant target permits a bounded forward step"),
		UGP_EnemyAttack::ShouldContinueAbilityForwardStep(0.2f, 0.65f, 100.0f, 500.0f, true, true, 300.0f, 180.0f));
	TestFalse(TEXT("Forward step stops at its authored duration"),
		UGP_EnemyAttack::ShouldContinueAbilityForwardStep(0.65f, 0.65f, 100.0f, 500.0f, true, true, 300.0f, 180.0f));
	TestFalse(TEXT("Forward step stops at its total travel budget"),
		UGP_EnemyAttack::ShouldContinueAbilityForwardStep(0.2f, 0.65f, 500.0f, 500.0f, true, true, 300.0f, 180.0f));
	TestFalse(TEXT("Forward step stops when the committed player is destroyed"),
		UGP_EnemyAttack::ShouldContinueAbilityForwardStep(0.2f, 0.65f, 100.0f, 500.0f, true, false, 300.0f, 180.0f));
	TestFalse(TEXT("Forward step stops before passing through a player already in hit reach"),
		UGP_EnemyAttack::ShouldContinueAbilityForwardStep(0.2f, 0.65f, 100.0f, 500.0f, true, true, 180.0f, 180.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyChaseAssetContractTest,
	"ProjectEden.AI.Enemy.ChaseAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyChaseAssetContractTest::RunTest(const FString& Parameters)
{
	UBehaviorTree* CommonTree = LoadObject<UBehaviorTree>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/BT/Common/BT_EnemyCommon.BT_EnemyCommon"));
	if (!TestNotNull(TEXT("Production common enemy behavior tree loads"), CommonTree))
	{
		return false;
	}

	const UBTTask_MoveTo* ChaseTask = EnemyAttackTransitionTests::FindMovingTargetChaseTask(CommonTree->RootNode);
	if (!TestNotNull(TEXT("Common tree chases the live TargetActor rather than a stale location"), ChaseTask))
	{
		return false;
	}

	// A null behavior component reads the asset's literal default without relying on deprecated implicit conversion.
	TestTrue(TEXT("Chase MoveTo tracks its moving actor goal"), ChaseTask->bTrackMovingGoal.GetValue(static_cast<const UBehaviorTreeComponent*>(nullptr)));
	TestTrue(TEXT("Chase acceptance radius reaches the melee close-hold edge"), ChaseTask->AcceptableRadius.GetValue(static_cast<const UBehaviorTreeComponent*>(nullptr)) < 225.0f);

	UClass* FurnaceClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/BP_FurnaceWalker.BP_FurnaceWalker_C"));
	UClass* CyclopsClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Characters/EnemyCharacter/Monsters/CyclopsSpecter/BP_CyclopsSpecter.BP_CyclopsSpecter_C"));
	const AGP_EnemyCharacter* FurnaceDefaults = IsValid(FurnaceClass)
		? Cast<AGP_EnemyCharacter>(FurnaceClass->GetDefaultObject())
		: nullptr;
	const AGP_EnemyCharacter* CyclopsDefaults = IsValid(CyclopsClass)
		? Cast<AGP_EnemyCharacter>(CyclopsClass->GetDefaultObject())
		: nullptr;
	if (TestNotNull(TEXT("Production Furnace defaults load"), FurnaceDefaults))
	{
		// Its bounded forward step supports the wider 350cm attack entry.
		TestTrue(TEXT("Furnace uses forward-step melee range"), FMath::IsNearlyEqual(FurnaceDefaults->GetBasicMeleeAttackStartRange(), 350.0f));
	}
	if (TestNotNull(TEXT("Production Cyclops defaults load"), CyclopsDefaults))
	{
		// Cyclops has no root motion or ability step, so its entry remains inside the shared overlap reach.
		TestTrue(TEXT("Cyclops uses in-place melee range"), FMath::IsNearlyEqual(CyclopsDefaults->GetBasicMeleeAttackStartRange(), 240.0f));
	}
	return true;
}

#endif
