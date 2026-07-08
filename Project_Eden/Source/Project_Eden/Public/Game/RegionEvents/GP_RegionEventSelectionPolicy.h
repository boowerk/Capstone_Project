#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventData.h"

namespace GPRegionEventSelectionPolicy
{
	/** Returns true when the authored event can participate in the requested trigger roll. */
	PROJECT_EDEN_API bool IsSelectable(const UGP_RegionEventData* EventData, EGPRegionEventTrigger Trigger);

	/** Adds every usable event weight so tests and runtime can share the same validation rule. */
	PROJECT_EDEN_API float GetTotalWeight(const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool, EGPRegionEventTrigger Trigger);

	/**
	 * Weighted picker for region events. Returning null is valid and means no authored event matched the trigger.
	 * The caller owns uniqueness/cooldown rules; this policy stays deterministic and data-only.
	 */
	PROJECT_EDEN_API const UGP_RegionEventData* SelectWeightedEvent(
		const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool,
		EGPRegionEventTrigger Trigger,
		FRandomStream& RandomStream);
}
