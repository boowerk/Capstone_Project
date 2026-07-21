#pragma once

#include "CoreMinimal.h"

class UGP_RegionEventData;

/** Pure selection rules for corruption-aware exploration events. */
namespace GPRegionEventExplorationPolicy
{
	// Converts local corruption into the probability of attempting an event on this evaluation tick.
	PROJECT_EDEN_API float CalculateAttemptChance(
		float NormalizedCorruption,
		float BaseChance,
		float FullCorruptionBonus);

	// Applies a readable bias: shrines favour cleaner regions while hostile events favour corrupted ones.
	PROJECT_EDEN_API float CalculateCorruptionWeight(
		const UGP_RegionEventData& EventData,
		float NormalizedCorruption);

	// Selects one eligible event and avoids immediately repeating the previous event when alternatives exist.
	PROJECT_EDEN_API const UGP_RegionEventData* SelectWeightedExplorationEvent(
		const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool,
		float NormalizedCorruption,
		FName PreviousEventId,
		FRandomStream& RandomStream);
}
