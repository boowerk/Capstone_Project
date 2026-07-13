#include "Game/RegionEvents/GP_RegionEventExplorationPolicy.h"

#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"

namespace
{
	float GetTypeBias(EGPRegionEventType EventType, float NormalizedCorruption)
	{
		const float Corruption = FMath::Clamp(NormalizedCorruption, 0.0f, 1.0f);
		switch (EventType)
		{
		case EGPRegionEventType::ShrineCache:
			// Rewards remain possible everywhere, but become rarer once a region is severely corrupted.
			return FMath::Lerp(1.65f, 0.30f, Corruption);
		case EGPRegionEventType::CorruptionRift:
			return FMath::Lerp(0.30f, 2.10f, Corruption);
		case EGPRegionEventType::EnvironmentalHazard:
			return FMath::Lerp(0.65f, 1.75f, Corruption);
		case EGPRegionEventType::EnemyAmbush:
		default:
			return FMath::Lerp(0.85f, 1.45f, Corruption);
		}
	}
}

float GPRegionEventExplorationPolicy::CalculateAttemptChance(
	float NormalizedCorruption,
	float BaseChance,
	float FullCorruptionBonus)
{
	// Clamping here keeps designer mistakes from turning the periodic roll into a guaranteed event storm.
	return FMath::Clamp(
		FMath::Max(0.0f, BaseChance)
			+ FMath::Clamp(NormalizedCorruption, 0.0f, 1.0f) * FMath::Max(0.0f, FullCorruptionBonus),
		0.0f,
		1.0f);
}

float GPRegionEventExplorationPolicy::CalculateCorruptionWeight(
	const UGP_RegionEventData& EventData,
	float NormalizedCorruption)
{
	const float Corruption = FMath::Clamp(NormalizedCorruption, 0.0f, 1.0f);
	if (Corruption < EventData.MinimumCorruptionNormalized
		|| Corruption > EventData.MaximumCorruptionNormalized)
	{
		return 0.0f;
	}

	return FMath::Max(0.0f, EventData.SelectionWeight) * GetTypeBias(EventData.EventType, Corruption);
}

const UGP_RegionEventData* GPRegionEventExplorationPolicy::SelectWeightedExplorationEvent(
	const TArray<TObjectPtr<UGP_RegionEventData>>& EventPool,
	float NormalizedCorruption,
	FName PreviousEventId,
	FRandomStream& RandomStream)
{
	TArray<const UGP_RegionEventData*> EligibleEvents;
	TArray<float> EligibleWeights;
	int32 NonRepeatedEventCount = 0;

	for (const UGP_RegionEventData* EventData : EventPool)
	{
		if (!IsValid(EventData) || !*EventData->EventActorClass)
		{
			continue;
		}

		const float Weight = CalculateCorruptionWeight(*EventData, NormalizedCorruption);
		if (Weight <= 0.0f)
		{
			continue;
		}

		EligibleEvents.Add(EventData);
		EligibleWeights.Add(Weight);
		NonRepeatedEventCount += EventData->EventId != PreviousEventId ? 1 : 0;
	}

	float TotalWeight = 0.0f;
	for (int32 Index = 0; Index < EligibleEvents.Num(); ++Index)
	{
		// If another choice exists, exclude the previous event to prevent visible streaks in short play sessions.
		if (NonRepeatedEventCount > 0 && EligibleEvents[Index]->EventId == PreviousEventId)
		{
			EligibleWeights[Index] = 0.0f;
		}
		TotalWeight += EligibleWeights[Index];
	}

	if (TotalWeight <= 0.0f)
	{
		return nullptr;
	}

	float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
	for (int32 Index = 0; Index < EligibleEvents.Num(); ++Index)
	{
		Roll -= EligibleWeights[Index];
		if (Roll <= 0.0f && EligibleWeights[Index] > 0.0f)
		{
			return EligibleEvents[Index];
		}
	}

	// Floating-point edge cases choose the last positive entry instead of silently dropping an event.
	for (int32 Index = EligibleEvents.Num() - 1; Index >= 0; --Index)
	{
		if (EligibleWeights[Index] > 0.0f)
		{
			return EligibleEvents[Index];
		}
	}

	return nullptr;
}
