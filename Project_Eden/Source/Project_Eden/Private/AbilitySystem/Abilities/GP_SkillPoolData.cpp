#include "AbilitySystem/Abilities/GP_SkillPoolData.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"

bool UGP_SkillPoolData::ContainsSkill(const UGP_SkillData* SkillData) const
{
	return IsValid(SkillData) && Skills.Contains(SkillData);
}

TArray<UGP_SkillData*> UGP_SkillPoolData::GetSkills() const
{
	TArray<UGP_SkillData*> Result;
	Result.Reserve(Skills.Num());

	for (UGP_SkillData* SkillData : Skills)
	{
		if (IsValid(SkillData))
		{
			Result.Add(SkillData);
		}
	}

	return Result;
}
