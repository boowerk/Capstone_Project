#include "Game/RegionEvents/GP_RegionEventSelectionPolicy.h"

namespace GPRegionEventSelectionPolicy
{
	bool IsSelectable(const UGP_RegionEventData* EventData, EGPRegionEventTrigger Trigger)
	{
		// Zero-weight assets stay in the pool as designer notes, but never roll at runtime.
		return IsValid(EventData)
			&& EventData->Trigger == Trigger
			&& EventData->SelectionWeight > 0.0f;
	}

	float GetTotalWeight(const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool, EGPRegionEventTrigger Trigger)
	{
		float TotalWeight = 0.0f;
		for (const TObjectPtr<UGP_RegionEventData>& EventData : EventPool)
		{
			if (IsSelectable(EventData, Trigger))
			{
				TotalWeight += EventData->SelectionWeight;
			}
		}

		return TotalWeight;
	}

	const UGP_RegionEventData* SelectWeightedEvent(
		const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool,
		EGPRegionEventTrigger Trigger,
		FRandomStream& RandomStream)
	{
		const float TotalWeight = GetTotalWeight(EventPool, Trigger);
		if (TotalWeight <= UE_KINDA_SMALL_NUMBER)
		{
			return nullptr;
		}

		float RemainingWeight = RandomStream.FRandRange(0.0f, TotalWeight);
		for (const TObjectPtr<UGP_RegionEventData>& EventData : EventPool)
		{
			if (!IsSelectable(EventData, Trigger))
			{
				continue;
			}

			RemainingWeight -= EventData->SelectionWeight;
			if (RemainingWeight <= 0.0f)
			{
				return EventData;
			}
		}

		// Floating point edge cases fall back to the last valid entry instead of silently skipping the event.
		for (int32 Index = EventPool.Num() - 1; Index >= 0; --Index)
		{
			const UGP_RegionEventData* EventData = EventPool[Index];
			if (IsSelectable(EventData, Trigger))
			{
				return EventData;
			}
		}

		return nullptr;
	}
}
