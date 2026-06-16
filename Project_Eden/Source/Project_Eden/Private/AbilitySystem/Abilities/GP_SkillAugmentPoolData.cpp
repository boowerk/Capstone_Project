#include "AbilitySystem/Abilities/GP_SkillAugmentPoolData.h"

#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"

TArray<UGP_SkillAugmentData*> UGP_SkillAugmentPoolData::PickRandomAugments(int32 Count) const
{
	TArray<UGP_SkillAugmentData*> EmptyExcludedAugments;
	return PickRandomAugmentsExcluding(Count, EmptyExcludedAugments);
}

TArray<UGP_SkillAugmentData*> UGP_SkillAugmentPoolData::PickRandomAugmentsExcluding(int32 Count, const TArray<UGP_SkillAugmentData*>& ExcludedAugments) const
{
	TArray<UGP_SkillAugmentData*> AvailableAugments;
	AvailableAugments.Reserve(Augments.Num());

	for (UGP_SkillAugmentData* Augment : Augments)
	{
		if (IsValid(Augment) && !ExcludedAugments.Contains(Augment))
		{
			AvailableAugments.Add(Augment);
		}
	}

	TArray<UGP_SkillAugmentData*> PickedAugments;
	const int32 PickCount = FMath::Min(FMath::Max(Count, 0), AvailableAugments.Num());
	PickedAugments.Reserve(PickCount);

	for (int32 PickIndex = 0; PickIndex < PickCount; ++PickIndex)
	{
		const int32 RandomIndex = FMath::RandRange(0, AvailableAugments.Num() - 1);
		PickedAugments.Add(AvailableAugments[RandomIndex]);
		AvailableAugments.RemoveAtSwap(RandomIndex);
	}

	return PickedAugments;
}

TArray<UGP_SkillAugmentData*> UGP_SkillAugmentPoolData::PickRandomAugmentsExcludingForElement(
	int32 Count,
	const TArray<UGP_SkillAugmentData*>& ExcludedAugments,
	FGameplayTag CurrentElementTag) const
{
	TArray<UGP_SkillAugmentData*> AvailableAugments;
	AvailableAugments.Reserve(Augments.Num());

	for (UGP_SkillAugmentData* Augment : Augments)
	{
		if (!IsValid(Augment) || ExcludedAugments.Contains(Augment))
		{
			continue;
		}

		if (Augment->RequiredElementTag.IsValid()
			&& !CurrentElementTag.MatchesTagExact(Augment->RequiredElementTag))
		{
			continue;
		}

		AvailableAugments.Add(Augment);
	}

	TArray<UGP_SkillAugmentData*> PickedAugments;
	const int32 PickCount = FMath::Min(FMath::Max(Count, 0), AvailableAugments.Num());
	PickedAugments.Reserve(PickCount);

	for (int32 PickIndex = 0; PickIndex < PickCount; ++PickIndex)
	{
		const int32 RandomIndex = FMath::RandRange(0, AvailableAugments.Num() - 1);
		PickedAugments.Add(AvailableAugments[RandomIndex]);
		AvailableAugments.RemoveAtSwap(RandomIndex);
	}

	return PickedAugments;
}
