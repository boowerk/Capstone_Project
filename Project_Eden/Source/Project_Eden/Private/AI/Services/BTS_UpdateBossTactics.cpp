#include "AI/Services/BTS_UpdateBossTactics.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"

namespace BTS_UpdateBossTactics_Internal
{
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
}

UBTS_UpdateBossTactics::UBTS_UpdateBossTactics()
{
	NodeName = TEXT("Update Boss Tactics");
}

void UBTS_UpdateBossTactics::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);
	UpdateBossTactics(OwnerComp);
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

	const bool bHasTarget = IsValid(Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor)));
	const bool bReturningHome = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReturnHome);
	const bool bCanAttack = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack);
	const bool bHasLineOfSight = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bHasLineOfSight);
	const float DistanceToTarget = BTS_UpdateBossTactics_Internal::GetOptionalBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget, 0.0f);
	const float WorldTimeSeconds = OwnerComp.GetWorld() != nullptr ? OwnerComp.GetWorld()->GetTimeSeconds() : 0.0f;

	// 페이즈 전환은 한 틱짜리 신호로 두어 BT가 전환 연출/버프 패턴을 우선 선택할 수 있게 한다.
	const bool bShouldPhaseTransition = bHasTarget && !bReturningHome && PreviousBossPhase > 0 && PreviousBossPhase != BossPhase;
	const bool bCanUseHeavyAttack = bHasTarget && !bReturningHome && !bShouldPhaseTransition && bCanAttack && DistanceToTarget <= HeavyAttackRange;
	const bool bCanUseAreaAttack = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& BossPhase >= 2
		&& bHasLineOfSight
		&& DistanceToTarget <= AreaAttackRange
		&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, AreaAttackInterval, AreaAttackWindow);
	const bool bCanUseSweepAttack = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& bCanAttack
		&& bHasLineOfSight
		// 실제 기본 공격/휘둘러치기 선택은 공격 태스크에서 공격 1회마다 랜덤으로 결정한다.
		&& DistanceToTarget <= SweepAttackRange;
	const bool bCanSummonAdds = bHasTarget
		&& !bReturningHome
		&& !bShouldPhaseTransition
		&& BossPhase >= 3
		&& BTS_UpdateBossTactics_Internal::IsPatternWindowOpen(WorldTimeSeconds, SummonInterval, SummonWindow);
	const bool bBossPatternRequestsAttack = bShouldPhaseTransition || bCanSummonAdds || bCanUseAreaAttack || bCanUseSweepAttack || bCanUseHeavyAttack;

	if (bBossPatternRequestsAttack)
	{
		// 기존 공용 BT의 공격 분기를 그대로 타도록, 보스 특수 패턴이 준비된 순간에는 이동/후퇴 분기보다 공격 분기를 우선시한다.
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanAttack, true);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldRetreat, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldReposition, false);
		BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldChase, false);
	}

	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bShouldBossPhaseTransition, bShouldPhaseTransition);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack, bCanUseHeavyAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack, bCanUseAreaAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossSweepAttack, bCanUseSweepAttack);
	BTS_UpdateBossTactics_Internal::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds, bCanSummonAdds);
}
