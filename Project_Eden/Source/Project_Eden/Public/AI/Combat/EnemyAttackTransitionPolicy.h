#pragma once

#include "CoreMinimal.h"

enum class EEnemyAttackTransitionIntent : uint8
{
	None,
	Attack,
	CombatHold
};

struct FEnemyAttackTransitionObservation
{
	bool bShouldRetreat = false;
	bool bHasLineOfSight = false;
	bool bInsideAttackBand = false;
	bool bAttackCadenceReady = false;
	bool bActionCommitted = false;
};

namespace EnemyAttackTransitionPolicy
{
	// 진입 범위보다 이탈 범위를 넓게 유지해 경계에서 Attack/Chase가 매 틱 왕복하지 않게 한다.
	PROJECT_EDEN_API bool IsInsideAttackBand(
		float DistanceToTarget,
		float MinAttackRange,
		float MaxAttackRange,
		bool bAllowAttacksInsidePreferredRange,
		bool bWasInsideAttackBand,
		float ExitHysteresis);

	// 한 프레임의 최대 회전량을 제한하면서 -180/180 경계를 최단 방향으로 통과한다.
	PROJECT_EDEN_API float StepFacingYaw(
		float CurrentYaw,
		float DesiredYaw,
		float TurnRateDegreesPerSecond,
		float DeltaSeconds);

	// 공격 의도와 실행 준비 시간을 분리해 쿨다운만으로 Chase가 열리지 않게 한다.
	// 타깃 소실·leash 재평가는 시작한 액션을 끊지 않고, 사망·그로기 같은 명시적 interrupt만 우선한다.
	PROJECT_EDEN_API bool ShouldPreserveCommittedAction(
		bool bActionCommitted,
		bool bExplicitInterrupt);

	// Personality tuning cannot expand a regular melee attack beyond its physical execution range.
	PROJECT_EDEN_API float ResolveMaximumAttackRange(
		float AuthoredMaximumRange,
		bool bIsRegularMelee,
		float MeleeMaximumStartRange);

	// Melee cadence recovery remains a close hold only while the target is already within pressure distance.
	PROJECT_EDEN_API bool ShouldPursueDuringCadenceRecovery(
		EEnemyAttackTransitionIntent Intent,
		bool bIsRegularMelee,
		bool bWasChasing,
		float DistanceToTarget,
		float MeleeHoldRange,
		float PursuitEntryHysteresis);

	PROJECT_EDEN_API EEnemyAttackTransitionIntent ResolveIntent(
		const FEnemyAttackTransitionObservation& Observation);
}
