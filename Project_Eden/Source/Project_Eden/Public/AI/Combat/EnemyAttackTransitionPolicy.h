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
	PROJECT_EDEN_API EEnemyAttackTransitionIntent ResolveIntent(
		const FEnemyAttackTransitionObservation& Observation);
}
