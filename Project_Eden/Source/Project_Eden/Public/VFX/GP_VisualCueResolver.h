#pragma once

#include "AbilitySystem/Abilities/GP_SkillData.h"

namespace GPVisualCueResolver
{
	/** Shared cue specificity rule used by player SkillData and actor-owned combat presentation. */
	inline UNiagaraSystem* ResolveNiagara(
		const TArray<FGP_SkillVisualCueEntry>& VisualCues,
		const FGameplayTag& ElementTag = FGameplayTag(),
		const FGameplayTag& CueTag = FGameplayTag())
	{
		UNiagaraSystem* BestNiagaraSystem = nullptr;
		int32 BestScore = INDEX_NONE;

		for (const FGP_SkillVisualCueEntry& Entry : VisualCues)
		{
			if (Entry.VisualType != EGP_SkillVisualType::Niagara || !Entry.NiagaraSystem)
			{
				continue;
			}

			const bool bCueMatches = !Entry.CueTag.IsValid() || (CueTag.IsValid() && Entry.CueTag.MatchesTagExact(CueTag));
			const bool bElementMatches = !Entry.ElementTag.IsValid() || (ElementTag.IsValid() && Entry.ElementTag.MatchesTagExact(ElementTag));
			if (!bCueMatches || !bElementMatches)
			{
				continue;
			}

			const int32 Score = (Entry.CueTag.IsValid() ? 2 : 0) + (Entry.ElementTag.IsValid() ? 1 : 0);
			if (Score > BestScore)
			{
				BestScore = Score;
				BestNiagaraSystem = Entry.NiagaraSystem;
			}
		}

		return BestNiagaraSystem;
	}
}
