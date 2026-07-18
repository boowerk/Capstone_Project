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
}
