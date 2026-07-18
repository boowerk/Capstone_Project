#include "AI/Combat/EnemyAttackTransitionPolicy.h"

namespace EnemyAttackTransitionPolicy
{
	bool IsInsideAttackBand(
		float DistanceToTarget,
		float MinAttackRange,
		float MaxAttackRange,
		bool bAllowAttacksInsidePreferredRange,
		bool bWasInsideAttackBand,
		float ExitHysteresis)
	{
		const float SafeDistance = FMath::Max(0.0f, DistanceToTarget);
		const float SafeMinRange = FMath::Max(0.0f, MinAttackRange);
		const float SafeMaxRange = FMath::Max(SafeMinRange, MaxAttackRange);
		const float AppliedHysteresis = bWasInsideAttackBand ? FMath::Max(0.0f, ExitHysteresis) : 0.0f;
		const float ExitMinRange = FMath::Max(0.0f, SafeMinRange - AppliedHysteresis);
		const float ExitMaxRange = SafeMaxRange + AppliedHysteresis;

		// 근접형은 안쪽 경계를 사용하지 않고, 원거리형만 너무 가까운 대상을 재배치로 넘긴다.
		return SafeDistance <= ExitMaxRange
			&& (bAllowAttacksInsidePreferredRange || SafeDistance >= ExitMinRange);
	}

	EEnemyAttackTransitionIntent ResolveIntent(const FEnemyAttackTransitionObservation& Observation)
	{
		if (Observation.bActionCommitted)
		{
			// 이미 시작한 공격은 전술 성향·거리·시야·쿨다운의 일시 변화보다 우선한다.
			return EEnemyAttackTransitionIntent::Attack;
		}

		if (Observation.bShouldRetreat)
		{
			return EEnemyAttackTransitionIntent::None;
		}

		if (!Observation.bHasLineOfSight || !Observation.bInsideAttackBand)
		{
			return EEnemyAttackTransitionIntent::None;
		}

		return Observation.bAttackCadenceReady
			? EEnemyAttackTransitionIntent::Attack
			: EEnemyAttackTransitionIntent::CombatHold;
	}
}
