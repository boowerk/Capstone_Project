#include "AI/Services/BTS_UpdateBossTactics.h"

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "AI/Tasks/BossAttackPatternSelector.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Characters/GP_DarkArmorKnightStateComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameplayTags/GP_Tags.h"

namespace BTS_UpdateBossTactics_Internal
{
	struct FTacticalStateSnapshot
	{
		bool bShouldRetreat = false;
		bool bCanAttack = false;
		bool bShouldReposition = false;
		bool bShouldChase = false;
		bool bShouldReturnHome = false;
	};

	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	void SetOptionalBlackboardBool(UBlackboardComponent* BlackboardComponent, const FName& KeyName, bool bValue)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsBool(KeyName, bValue);
		}
	}

	void SetOptionalBlackboardInt(UBlackboardComponent* BlackboardComponent, const FName& KeyName, int32 Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsInt(KeyName, Value);
		}
	}

	void SetOptionalBlackboardFloat(UBlackboardComponent* BlackboardComponent, const FName& KeyName, float Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsFloat(KeyName, Value);
		}
	}

	void SetOptionalBlackboardObject(UBlackboardComponent* BlackboardComponent, const FName& KeyName, UObject* Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsObject(KeyName, Value);
		}
	}

	bool GetOptionalBlackboardBool(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) && BlackboardComponent->GetValueAsBool(KeyName);
	}

	float GetOptionalBlackboardFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsFloat(KeyName) : DefaultValue;
	}

	bool IsPatternWindowOpen(float WorldTimeSeconds, float Interval, float Window)
	{
		if (Interval <= KINDA_SMALL_NUMBER)
		{
			return true;
		}

		return FMath::Fmod(FMath::Max(0.0f, WorldTimeSeconds), Interval) <= FMath::Max(0.0f, Window);
	}

	bool IsDistanceInRange(float Distance, float MinRange, float MaxRange)
	{
		const float SafeDistance = FMath::Max(0.0f, Distance);
		return SafeDistance >= FMath::Max(0.0f, MinRange) && SafeDistance <= FMath::Max(MinRange, MaxRange);
	}

	void SetOptionalBlackboardName(UBlackboardComponent* BlackboardComponent, const FName& KeyName, FName Value)
	{
		if (HasBlackboardKey(BlackboardComponent, KeyName))
		{
			BlackboardComponent->SetValueAsName(KeyName, Value);
		}
	}

	FTacticalStateSnapshot CaptureTacticalState(const UBlackboardComponent* BlackboardComponent)
	{
		FTacticalStateSnapshot Snapshot;
		if (!IsValid(BlackboardComponent))
		{
			return Snapshot;
		}

		Snapshot.bShouldRetreat = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldRetreat);
		Snapshot.bCanAttack = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bCanAttack);
		Snapshot.bShouldReposition = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReposition);
		Snapshot.bShouldChase = BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldChase);
		Snapshot.bShouldReturnHome = GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
		return Snapshot;
	}

	void RequestRootReevaluationIfStateChanged(
		UBehaviorTreeComponent& OwnerComp,
		const FTacticalStateSnapshot& PreviousState)
	{
		const FTacticalStateSnapshot CurrentState = CaptureTacticalState(OwnerComp.GetBlackboardComponent());
		const bool bStateChanged = PreviousState.bShouldRetreat != CurrentState.bShouldRetreat
			|| PreviousState.bCanAttack != CurrentState.bCanAttack
			|| PreviousState.bShouldReposition != CurrentState.bShouldReposition
			|| PreviousState.bShouldChase != CurrentState.bShouldChase
			|| PreviousState.bShouldReturnHome != CurrentState.bShouldReturnHome;
		if (!bStateChanged || !OwnerComp.IsRunning() || OwnerComp.IsRestartPending() || OwnerComp.IsAbortPending())
		{
			return;
		}

		if (AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner()))
		{
			// Defer the restart because services execute inside the Behavior Tree component tick.
			EnemyAIController->RequestBehaviorTreeRootReevaluation();
		}
	}
}

UBTS_UpdateBossTactics::UBTS_UpdateBossTactics()
{
	NodeName = TEXT("Update Boss Tactics");
	// The derived service publishes the final state after shared observations are refreshed.
	bRestartTreeOnTacticalStateChange = false;
}

void UBTS_UpdateBossTactics::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	const BTS_UpdateBossTactics_Internal::FTacticalStateSnapshot PreviousState =
		BTS_UpdateBossTactics_Internal::CaptureTacticalState(OwnerComp.GetBlackboardComponent());
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateBossTactics(OwnerComp);
	BTS_UpdateBossTactics_Internal::RequestRootReevaluationIfStateChanged(OwnerComp, PreviousState);
}

void UBTS_UpdateBossTactics::OnSearchStart(FBehaviorTreeSearchData& SearchData)
{
	Super::OnSearchStart(SearchData);
	UpdateBossTactics(SearchData.OwnerComp);
}

FString UBTS_UpdateBossTactics::GetStaticServiceDescription() const
{
	return FString::Printf(
		TEXT("%s\nBoss phases %.0f%% / %.0f%%, AoE %.1fs, Sweep random <= %.0fcm, Summon %.1fs"),
		*Super::GetStaticServiceDescription(),
		PhaseTwoHealthRatio * 100.0f,
		PhaseThreeHealthRatio * 100.0f,
		AreaAttackInterval,
		SweepAttackRange,
		SummonInterval);
}

void UBTS_UpdateBossTactics::UpdateBossTactics(UBehaviorTreeComponent& OwnerComp) const
{
	UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	const int32 PreviousBossPhase = BTS_UpdateBossTactics_Internal::HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::BossPhase)
		? BlackboardComponent->GetValueAsInt(EnemyBlackboardKeys::BossPhase)
		: 0;

	const float HealthRatio = FMath::Clamp(BTS_UpdateBossTactics_Internal::GetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::HealthRatio, 1.0f), 0.0f, 1.0f);
	const int32 BossPhase = HealthRatio <= PhaseThreeHealthRatio ? 3 : (HealthRatio <= PhaseTwoHealthRatio ? 2 : 1);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardInt(BlackboardComponent, EnemyBlackboardKeys::BossPhase, BossPhase);

	AAIController* AIController = OwnerComp.GetAIOwner();
	APawn* ControlledPawn = IsValid(AIController) ? AIController->GetPawn() : nullptr;
	AActor* TargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	const bool bHasTarget = IsValid(TargetActor);
	const bool bReturningHome = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
	const bool bShouldReposition = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition);
	const bool bHasLineOfSight = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bHasLineOfSight);
	const float DistanceToTarget = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget, 0.0f);
	const float WorldTimeSeconds = OwnerComp.GetWorld() != nullptr ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;
	const AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(OwnerComp.GetAIOwner());
	const FName EnemyMode = BlackboardComponent->GetValueAsName(EnemyBlackboardKeys::EnemyMode);
	const float PreferredRange = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, 600.0f);
	const float EffectiveBasicReach = FMath::Min(HeavyAttackRange, FGPBossAttackPatternRanges::BasicAttackReach);
	const float EffectiveSweepReach = FMath::Min(SweepAttackRange, FGPBossAttackPatternRanges::SweepAttackReach);
	const float EffectiveAreaReach = FMath::Min(AreaAttackRange, FGPBossAttackPatternRanges::AreaAttackReach);
	const bool bBasicAttackCanReach = FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, EffectiveBasicReach);
	const bool bSweepAttackCanReach = FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, EffectiveSweepReach);
	const bool bAreaAttackCanReach = FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, EffectiveAreaReach);

	// 페이즈 전환은 한 틱짜리 신호로 두어 BT가 전환 연출/버프 패턴을 우선 선택할 수 있게 한다.
	bool bShouldPhaseTransition = bHasTarget && !bReturningHome && PreviousBossPhase > 0 && PreviousBossPhase != BossPhase;
	bool bCanUseHeavyAttack = bHasTarget && !bReturningHome && !bShouldPhaseTransition && bHasLineOfSight && bBasicAttackCanReach;
	bool bCanUseAreaAttack = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& BossPhase >= 2
		&& bHasLineOfSight
		&& bAreaAttackCanReach
		&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, AreaAttackInterval, AreaAttackWindow);
	bool bCanUseSweepAttack = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& bHasLineOfSight
		// 실제 기본 공격/휘둘러치기 선택은 공격 태스크에서 실시간 평가 점수로 결정한다.
		&& bSweepAttackCanReach;
	bool bCanSummonAdds = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		// Let the summon pattern enter from phase 2 so it is visible before the fight is nearly over.
		&& BossPhase >= 2
		&& (bBasicAttackCanReach || bSweepAttackCanReach || bAreaAttackCanReach)
		&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, SummonInterval, SummonWindow);

	const UGP_MatadorBossStateComponent* MatadorStateComponent = IsValid(ControlledPawn)
		? ControlledPawn->FindComponentByClass<UGP_MatadorBossStateComponent>()
		: nullptr;
	const AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(ControlledPawn);
	const int32 ChainBreakCount = IsValid(MatadorStateComponent) ? MatadorStateComponent->GetChainBreakCount() : 0;
	const int32 ChainBreakTarget = IsValid(MatadorStateComponent) ? MatadorStateComponent->GetChainBreakTarget() : 3;
	const bool bMatadorGroggy = IsValid(MatadorStateComponent) && MatadorStateComponent->IsGroggy();
	// The Matador reports both a live bull and a bull reserved behind the optional VFX lead-in.
	const bool bBullPatternActive = IsValid(MatadorBoss)
		? MatadorBoss->IsBullPatternActive()
		: (IsValid(MatadorStateComponent) && IsValid(MatadorStateComponent->GetActiveBullActor()));
	const float MatadorPreferredHoverHeight = IsValid(MatadorBoss) ? MatadorBoss->GetPreferredHoverHeight() : PreferredHoverHeight;
	const float MatadorPreferredAirRange = IsValid(MatadorBoss) ? MatadorBoss->GetPreferredAirRange() : PreferredAirRange;
	const bool bIsMatadorBoss = IsValid(MatadorBoss);
	const bool bBullRangeAllowed = DistanceToTarget >= BullPatternMinRange && DistanceToTarget <= BullPatternMaxRange;
	const bool bCanUseBullPattern = IsValid(MatadorStateComponent)
		&& bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& !bMatadorGroggy
		&& !bBullPatternActive
		&& (bHasLineOfSight || bIsMatadorBoss)
		&& bBullRangeAllowed
		&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, BullPatternInterval, BullPatternWindow);
	const bool bCanTriggerMatadorGroggy = IsValid(MatadorStateComponent) && ChainBreakCount >= ChainBreakTarget && !bMatadorGroggy;
	const bool bShouldTeleport = IsValid(MatadorBoss) && bHasTarget && !bReturningHome && MatadorBoss->ShouldTeleportForMatador(DistanceToTarget);
	bool bCanUseMatadorMeleePattern = false;
	bool bMatadorForceRangeReposition = false;

	const UGP_CrystalSeraphStateComponent* CrystalSeraphStateComponent = IsValid(ControlledPawn)
		? ControlledPawn->FindComponentByClass<UGP_CrystalSeraphStateComponent>()
		: nullptr;
	const AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss = Cast<AGP_CrystalSeraphBossCharacter>(ControlledPawn);
	const bool bIsCrystalSeraphBoss = IsValid(CrystalSeraphBoss);
	const int32 WingCoreBreakCount = IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetWingCoreBreakCount() : 0;
	const int32 WingCoreBreakTarget = IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetWingCoreBreakTarget() : 3;
	const bool bCrystalSeraphGroggy = IsValid(CrystalSeraphStateComponent) && CrystalSeraphStateComponent->IsGroggy();
	const bool bWingCoreExposed = IsValid(CrystalSeraphStateComponent) && CrystalSeraphStateComponent->IsWingCoreExposed();
	const bool bCrystalPrismActive = IsValid(CrystalSeraphStateComponent) && IsValid(CrystalSeraphStateComponent->GetCrystalPrismActor());
	const bool bCanTriggerCrystalSeraphGroggy = IsValid(CrystalSeraphStateComponent) && WingCoreBreakCount >= WingCoreBreakTarget && !bCrystalSeraphGroggy;
	const float CrystalPreferredHoverHeight = IsValid(CrystalSeraphBoss) ? CrystalSeraphBoss->GetPreferredHoverHeight() : PreferredHoverHeight;
	const float CrystalPreferredAirRange = IsValid(CrystalSeraphBoss) ? CrystalSeraphBoss->GetPreferredAirRange() : PreferredAirRange;
	// Actor-owned timestamps keep prism and laser eligible until used instead of depending on a narrow global clock window.
	const bool bCrystalPatternIntervalReady = IsValid(CrystalSeraphBoss) && CrystalSeraphBoss->CanStartCrystalSeraphPattern();
	const bool bCrystalLaserCooldownReady = IsValid(CrystalSeraphBoss)
		&& WorldTimeSeconds - CrystalSeraphBoss->GetLastLaserPatternTime() >= CrystalSeraphBoss->GetLaserPatternCooldown();
	const bool bCrystalPrismCooldownReady = IsValid(CrystalSeraphBoss)
		&& WorldTimeSeconds - CrystalSeraphBoss->GetLastPrismPatternTime() >= CrystalSeraphBoss->GetPrismPatternCooldown();
	const bool bCrystalLaserRangeAllowed = BTS_UpdateBossTactics_Internal::IsDistanceInRange(DistanceToTarget, CrystalLaserMinRange, CrystalLaserMaxRange);
	const bool bCrystalPrismRangeAllowed = BTS_UpdateBossTactics_Internal::IsDistanceInRange(DistanceToTarget, CrystalPrismMinRange, CrystalPrismMaxRange);
	bool bCanUseCrystalLaserPattern = IsValid(CrystalSeraphStateComponent)
		&& bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& !bCrystalSeraphGroggy
		&& !bWingCoreExposed
		&& bCrystalPrismActive
		&& bHasLineOfSight
		&& bCrystalLaserRangeAllowed
		&& bCrystalPatternIntervalReady
		&& bCrystalLaserCooldownReady;
	bool bCanUseCrystalPrismPattern = IsValid(CrystalSeraphStateComponent)
		&& bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& !bCrystalSeraphGroggy
		&& !bWingCoreExposed
		&& !bCrystalPrismActive
		&& bCrystalPrismRangeAllowed
		&& bCrystalPatternIntervalReady
		&& bCrystalPrismCooldownReady;
	const bool bCrystalShouldTeleport = IsValid(CrystalSeraphBoss)
		&& bHasTarget
		&& !bReturningHome
		&& !bCrystalSeraphGroggy
		&& !bWingCoreExposed
		&& (CrystalSeraphBoss->ShouldTeleportForCrystalSeraph(DistanceToTarget) || !bHasLineOfSight);
	const bool bCanUseCrystalTeleportPattern = bCrystalShouldTeleport && bCrystalPatternIntervalReady;
	const bool bCanUseCrystalBasicPattern = IsValid(CrystalSeraphStateComponent)
		&& bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& !bCrystalSeraphGroggy
		&& !bWingCoreExposed
		&& !bCrystalShouldTeleport
		&& bHasLineOfSight
		&& bCrystalPatternIntervalReady
		&& DistanceToTarget <= FMath::Max(0.0f, CrystalShardMaxRange);

	UGP_DarkArmorKnightStateComponent* DarkKnightStateComponent = IsValid(ControlledPawn)
		? ControlledPawn->FindComponentByClass<UGP_DarkArmorKnightStateComponent>()
		: nullptr;
	const AGP_DarkArmorKnightBossCharacter* DarkKnightBoss = Cast<AGP_DarkArmorKnightBossCharacter>(ControlledPawn);
	const bool bIsDarkKnightBoss = IsValid(DarkKnightBoss);
	const bool bDarkKnightGroggy = IsValid(DarkKnightStateComponent) && DarkKnightStateComponent->IsGroggy();
	const bool bDarkKnightGuarding = IsValid(DarkKnightStateComponent) && DarkKnightStateComponent->IsGuarding();
	const bool bDarkKnightGuardBroken = IsValid(DarkKnightStateComponent) && DarkKnightStateComponent->IsGuardBroken();
	if (bIsDarkKnightBoss)
	{
		// Dark Knight changes phases through its own 60/25 percent rules, so the common 66/33 transition signal must not block a pattern tick.
		bShouldPhaseTransition = false;
	}
	if (IsValid(DarkKnightStateComponent))
	{
		// The state component is authoritative; Blackboard phase remains only a readable BT mirror.
		DarkKnightStateComponent->SetCombatPhase(IsValid(DarkKnightBoss) ? DarkKnightBoss->GetDarkKnightPhase() : BossPhase);
	}
	const bool bDarkKnightCadenceReady = bIsDarkKnightBoss && DarkKnightBoss->CanStartDarkKnightPattern();
	const bool bDarkKnightMeleeReady = bDarkKnightCadenceReady
		&& DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightMeleeReach
		&& (DarkKnightBoss->IsPatternCooldownReady(GPTags::Ability::Boss::DarkKnight::Basic)
			|| DarkKnightBoss->IsPatternCooldownReady(GPTags::Ability::Boss::DarkKnight::Heavy));
	const bool bDarkKnightChargeReady = bDarkKnightCadenceReady
		&& DistanceToTarget >= DarkKnightBoss->GetChargeMinRange()
		&& DistanceToTarget <= FGPBossAttackPatternRanges::DarkKnightChargeMaxRange
		&& DarkKnightBoss->IsPatternCooldownReady(GPTags::Ability::Boss::DarkKnight::Charge);
	const bool bDarkKnightWaveReady = bDarkKnightCadenceReady
		&& DistanceToTarget <= DarkKnightBoss->GetDarkWaveMaxRange()
		&& DarkKnightBoss->IsPatternCooldownReady(GPTags::Ability::Boss::DarkKnight::DarkWave);
	const bool bDarkKnightCrackReady = bDarkKnightCadenceReady
		&& DistanceToTarget <= DarkKnightBoss->GetGroundCrackMaxRange()
		&& DarkKnightBoss->IsPatternCooldownReady(GPTags::Ability::Boss::DarkKnight::GroundCrack);
	const bool bDarkKnightCanAttackAtCurrentRange = bDarkKnightGuardBroken || bDarkKnightMeleeReady;
	const bool bCanUseDarkKnightPattern = bIsDarkKnightBoss
		&& bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& !bDarkKnightGroggy
		&& bHasLineOfSight
		&& bDarkKnightCanAttackAtCurrentRange;

	if (bMatadorGroggy)
	{
		// Groggy makes the boss vulnerable; suppress normal attack requests until the state component recovers it.
		bShouldPhaseTransition = false;
		bCanUseHeavyAttack = false;
		bCanUseAreaAttack = false;
		bCanUseSweepAttack = false;
		bCanSummonAdds = false;
	}

	if (bIsMatadorBoss && !bMatadorGroggy)
	{
		// Matador stays mage-like at range and uses bull/decoy instead of generic melee spam.
		bCanUseHeavyAttack = false;
		bCanUseSweepAttack = false;
		bCanSummonAdds = false;
		bCanUseAreaAttack = false;

		const bool bInMatadorCapeGustRange = FGPBossAttackPatternRanges::IsWithinReach(DistanceToTarget, FGPBossAttackPatternRanges::MatadorCapeGustReach);
		const bool bInMatadorRapierRange = FGPBossAttackPatternRanges::IsWithinMatadorRapierRange(DistanceToTarget);
		bCanUseMatadorMeleePattern = bHasTarget
			&& !bReturningHome
			&& !bShouldPhaseTransition
			&& !bCanUseBullPattern
			&& !bBullPatternActive
			&& (bInMatadorCapeGustRange || bInMatadorRapierRange);

		if (!bCanUseBullPattern && !bCanUseMatadorMeleePattern && DistanceToTarget < MatadorPreferredAirRange * 0.75f)
		{
			bMatadorForceRangeReposition = true;
		}
	}

	if (bCrystalSeraphGroggy)
	{
		// Groggy makes Crystal Seraph a reward window; suppress every attack selector input.
		bShouldPhaseTransition = false;
		bCanUseHeavyAttack = false;
		bCanUseAreaAttack = false;
		bCanUseSweepAttack = false;
		bCanSummonAdds = false;
		bCanUseCrystalLaserPattern = false;
		bCanUseCrystalPrismPattern = false;
	}

	if (bIsCrystalSeraphBoss && !bCrystalSeraphGroggy)
	{
		// Crystal Seraph prefers air-range shard, prism, laser, and sanctuary patterns over generic melee/summon pressure.
		bCanUseHeavyAttack = false;
		bCanUseSweepAttack = false;
		bCanSummonAdds = false;
		bCanUseAreaAttack = bHasTarget
			&& !bReturningHome
			&& !bShouldPhaseTransition
			&& !bWingCoreExposed
			&& bCrystalPatternIntervalReady
			&& BossPhase >= 2
			&& bHasLineOfSight
			&& BTS_UpdateBossTactics_Internal::IsDistanceInRange(DistanceToTarget, CrystalPreferredAirRange * 0.5f, AreaAttackRange)
			&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, AreaAttackInterval, AreaAttackWindow);

	}

	if (bIsDarkKnightBoss)
	{
		// Dedicated Dark Knight tags own every attack; never leak Sans/common patterns into this boss.
		bCanUseHeavyAttack = false;
		bCanUseAreaAttack = false;
		bCanUseSweepAttack = false;
		bCanSummonAdds = false;
		bShouldPhaseTransition = false;
	}

	if (!bMatadorGroggy && !bCrystalSeraphGroggy && IsValid(EnemyAIController) && EnemyAIController->IsBossRuntimeEvaluationTestCycleActive() && bHasTarget && !bReturningHome && bHasLineOfSight)
	{
		const bool bModePrefersHold = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Hold);
		const bool bTestAreaWindow = bModePrefersHold && PreferredRange <= 250.0f;
		// Test cycle bypasses phase/time gates but still refuses attacks outside hit reach.
		bShouldPhaseTransition = false;
		bCanUseHeavyAttack = bModePrefersHold && !bTestAreaWindow && bBasicAttackCanReach;
		bCanUseAreaAttack = bTestAreaWindow && bAreaAttackCanReach;
		bCanUseSweepAttack = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Pressure) && bSweepAttackCanReach;
		bCanSummonAdds = EnemyMode == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyMode::Retreat) && (bBasicAttackCanReach || bSweepAttackCanReach || bAreaAttackCanReach);
	}

	const bool bBossPatternRequestsAttack = bShouldPhaseTransition
		|| bCanTriggerMatadorGroggy
		|| bCanTriggerCrystalSeraphGroggy
		|| bCanUseBullPattern
		|| bCanUseMatadorMeleePattern
		|| bCanUseCrystalLaserPattern
		|| bCanUseCrystalPrismPattern
		|| bCanUseCrystalTeleportPattern
		|| bCanUseCrystalBasicPattern
		|| bCanUseDarkKnightPattern
		|| bCanSummonAdds
		|| bCanUseAreaAttack
		|| bCanUseSweepAttack
		|| bCanUseHeavyAttack;

	if (bMatadorForceRangeReposition)
	{
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, true);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, true);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}
	else if (bBossPatternRequestsAttack)
	{
		// 기존 공용 BT의 공격 분기를 그대로 타도록, 보스 특수 패턴이 준비된 순간에는 이동/후퇴 분기보다 공격 분기를 우선시한다.
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, true);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}
	else if (bMatadorGroggy || bCrystalSeraphGroggy || bDarkKnightGroggy || (bIsCrystalSeraphBoss && bWingCoreExposed))
	{
		// Vulnerability windows are stationary and must not fall through to the shared Chase branch.
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}
	else if (bHasTarget && !bReturningHome && !bShouldReposition)
	{
		// If no boss attack can reach, keep the boss moving instead of playing whiffed attacks.
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, true);
	}
	else if (bIsMatadorBoss || bIsCrystalSeraphBoss || bIsDarkKnightBoss)
	{
		// Never inherit a stale generic attack request while a boss is returning home or already repositioning.
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, false);
	}

	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldBossPhaseTransition, bShouldPhaseTransition);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack, bCanUseHeavyAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack, bCanUseAreaAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossSweepAttack, bCanUseSweepAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds, bCanSummonAdds);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardInt(BlackboardComponent, EnemyBlackboardKeys::ChainBreakCount, ChainBreakCount);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bIsGroggy, bMatadorGroggy || bCrystalSeraphGroggy || bDarkKnightGroggy);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBullPattern, bCanUseBullPattern);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bBullPatternActive, bBullPatternActive);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardObject(BlackboardComponent, EnemyBlackboardKeys::DecoyActor, IsValid(MatadorStateComponent) ? MatadorStateComponent->GetDecoyActor() : nullptr);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardObject(BlackboardComponent, EnemyBlackboardKeys::MainBossActor,
		IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetMainBossActor() : (IsValid(MatadorStateComponent) ? MatadorStateComponent->GetMainBossActor() : ControlledPawn));
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredHoverHeight, bIsCrystalSeraphBoss ? CrystalPreferredHoverHeight : MatadorPreferredHoverHeight);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredAirRange, bIsCrystalSeraphBoss ? CrystalPreferredAirRange : MatadorPreferredAirRange);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldTeleport, bIsCrystalSeraphBoss ? bCrystalShouldTeleport : bShouldTeleport);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardInt(BlackboardComponent, EnemyBlackboardKeys::WingCoreBreakCount, WingCoreBreakCount);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanExposeWingCore, IsValid(CrystalSeraphStateComponent) && !bWingCoreExposed && !bCrystalSeraphGroggy);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bWingCoreExposed, bWingCoreExposed);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardObject(BlackboardComponent, EnemyBlackboardKeys::CrystalPrismActor, IsValid(CrystalSeraphStateComponent) ? CrystalSeraphStateComponent->GetCrystalPrismActor() : nullptr);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseLaserPattern, bCanUseCrystalLaserPattern);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUsePrismPattern, bCanUseCrystalPrismPattern);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::GuardGauge, IsValid(DarkKnightStateComponent) ? DarkKnightStateComponent->GetGuardGauge() : 0.0f);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::MaxGuardGauge, IsValid(DarkKnightStateComponent) ? DarkKnightStateComponent->GetMaxGuardGauge() : 0.0f);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bIsGuarding, bDarkKnightGuarding);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanParry, IsValid(DarkKnightStateComponent) && DarkKnightStateComponent->IsParryWindowOpen());
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bGuardBroken, bDarkKnightGuardBroken);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldCharge, bCanUseDarkKnightPattern && bDarkKnightChargeReady);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseGroundCrack, bCanUseDarkKnightPattern && bDarkKnightCrackReady);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseDarkWave, bCanUseDarkKnightPattern && bDarkKnightWaveReady);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardName(BlackboardComponent, EnemyBlackboardKeys::LastHitDirection, IsValid(DarkKnightStateComponent) ? DarkKnightStateComponent->GetLastHitDirectionName() : NAME_None);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredMeleeRange, IsValid(DarkKnightBoss) ? DarkKnightBoss->GetPreferredMeleeRange() : 350.0f);
}
