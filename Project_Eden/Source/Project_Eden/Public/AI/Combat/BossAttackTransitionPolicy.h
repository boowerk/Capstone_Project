#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Pure timing policy for boss patterns whose GAS ability hands execution to an
 * actor-owned timer or coordinator before the visible action has finished.
 */
namespace BossAttackTransitionPolicy
{
	PROJECT_EDEN_API float ResolvePostAbilityCommitSeconds(
		const FGameplayTag& PatternTag,
		float DefaultCommitSeconds);

	// Actor-owned external action은 실제 종료 신호를 우선하고, 별도 stuck cap에서만 강제로 해제한다.
	PROJECT_EDEN_API bool ShouldCompleteExternalAction(
		bool bExternalActionActive,
		float TotalElapsedSeconds,
		float ExternalActionTimeoutSeconds);

	// The BT attack branch opens when any currently-reachable Dark Knight
	// pattern is ready, rather than requiring a melee candidate.
	PROJECT_EDEN_API bool CanRequestDarkKnightPattern(
		bool bGuardBroken,
		bool bMeleeReady,
		bool bChargeReady,
		bool bDarkWaveReady,
		bool bGroundCrackReady);
}
