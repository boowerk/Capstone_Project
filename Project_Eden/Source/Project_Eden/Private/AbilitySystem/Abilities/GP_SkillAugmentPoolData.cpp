#include "AbilitySystem/Abilities/GP_SkillAugmentPoolData.h"

#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"

TArray<UGP_SkillAugmentData*> UGP_SkillAugmentPoolData::PickRandomAugments(int32 Count) const
{
	TArray<UGP_SkillAugmentData*> AvailableAugments;
	AvailableAugments.Reserve(Augments.Num());

	for (UGP_SkillAugmentData* Augment : Augments)
	{
		if (IsValid(Augment))
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
