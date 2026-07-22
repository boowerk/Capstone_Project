#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"

#include "Misc/Crc.h"

namespace
{
	void AddWarning(FGP_VillageSelectionResult& Result, const FString& Message, bool bIsError)
	{
		Result.Warnings.Add(Message);
		if (bIsError)
		{
			Result.bSucceeded = false;
		}
	}

	int32 MakeGroupSeed(int32 RunSeed, FName GroupId)
	{
		const uint32 GroupHash = FCrc::StrCrc32(*GroupId.ToString());
		return static_cast<int32>(HashCombineFast(static_cast<uint32>(RunSeed), GroupHash) & static_cast<uint32>(MAX_int32));
	}

	int32 SelectWeightedIndex(const TArray<FGP_VillageCandidate>& Candidates, FRandomStream& RandomStream)
	{
		float TotalWeight = 0.0f;
		for (const FGP_VillageCandidate& Candidate : Candidates)
		{
			TotalWeight += Candidate.SelectionWeight;
		}

		if (TotalWeight <= 0.0f)
		{
			return INDEX_NONE;
		}

		float Roll = RandomStream.FRandRange(0.0f, TotalWeight);
		for (int32 CandidateIndex = 0; CandidateIndex < Candidates.Num(); ++CandidateIndex)
		{
			Roll -= Candidates[CandidateIndex].SelectionWeight;
			if (Roll <= 0.0f)
			{
				return CandidateIndex;
			}
		}

		return Candidates.Num() - 1;
	}
}

FGP_VillageSelectionResult GPVillageSelectionPolicy::SelectSlots(
	int32 RunSeed,
	const TArray<FGP_VillageCandidate>& Candidates,
	const TArray<FGP_VillageGroupRule>& GroupRules)
{
	FGP_VillageSelectionResult Result;
	if (RunSeed < 0)
	{
		AddWarning(Result, TEXT("RunSeed must be non-negative."), true);
		return Result;
	}

	if (GroupRules.IsEmpty())
	{
		AddWarning(Result, TEXT("No village group rules are configured."), true);
		return Result;
	}

	TMap<FName, int32> SlotIdCounts;
	for (const FGP_VillageCandidate& Candidate : Candidates)
	{
		if (!Candidate.SlotId.IsNone())
		{
			SlotIdCounts.FindOrAdd(Candidate.SlotId)++;
		}
	}

	TArray<FGP_VillageCandidate> ValidCandidates;
	for (const FGP_VillageCandidate& Candidate : Candidates)
	{
		if (Candidate.SlotId.IsNone() || Candidate.GroupId.IsNone())
		{
			AddWarning(Result, TEXT("Village candidate has an empty SlotId or GroupId and was ignored."), true);
			continue;
		}

		if (SlotIdCounts.FindRef(Candidate.SlotId) > 1)
		{
			AddWarning(Result,
				FString::Printf(TEXT("Duplicate village SlotId '%s' was ignored."), *Candidate.SlotId.ToString()),
				true);
			continue;
		}

		if (Candidate.SelectionWeight <= 0.0f)
		{
			continue;
		}

		ValidCandidates.Add(Candidate);
	}

	ValidCandidates.Sort([](const FGP_VillageCandidate& A, const FGP_VillageCandidate& B)
	{
		return A.GroupId == B.GroupId ? A.SlotId.LexicalLess(B.SlotId) : A.GroupId.LexicalLess(B.GroupId);
	});

	TMap<FName, int32> GroupRuleCounts;
	for (const FGP_VillageGroupRule& Rule : GroupRules)
	{
		if (!Rule.GroupId.IsNone())
		{
			GroupRuleCounts.FindOrAdd(Rule.GroupId)++;
		}
	}

	TArray<FGP_VillageGroupRule> SortedRules = GroupRules;
	SortedRules.Sort([](const FGP_VillageGroupRule& A, const FGP_VillageGroupRule& B)
	{
		return A.GroupId.LexicalLess(B.GroupId);
	});

	TSet<FName> ConfiguredGroups;
	for (const FGP_VillageGroupRule& Rule : SortedRules)
	{
		if (Rule.GroupId.IsNone())
		{
			AddWarning(Result, TEXT("Village group rule has an empty GroupId and was ignored."), true);
			continue;
		}

		if (GroupRuleCounts.FindRef(Rule.GroupId) > 1)
		{
			if (!ConfiguredGroups.Contains(Rule.GroupId))
			{
				AddWarning(Result,
					FString::Printf(TEXT("Duplicate village group rule '%s' was ignored."), *Rule.GroupId.ToString()),
					true);
				ConfiguredGroups.Add(Rule.GroupId);
			}
			continue;
		}

		ConfiguredGroups.Add(Rule.GroupId);
		if (!Rule.bUsePickCountRange && Rule.PickCount <= 0)
		{
			AddWarning(Result,
				FString::Printf(TEXT("Village group '%s' has an invalid PickCount."), *Rule.GroupId.ToString()),
				true);
			continue;
		}
		if (Rule.bUsePickCountRange
			&& (Rule.MinPickCount < 1 || Rule.MaxPickCount < Rule.MinPickCount))
		{
			AddWarning(Result,
				FString::Printf(TEXT("Village group '%s' has an invalid pick-count range."), *Rule.GroupId.ToString()),
				true);
			continue;
		}

		TArray<FGP_VillageCandidate> GroupCandidates;
		for (const FGP_VillageCandidate& Candidate : ValidCandidates)
		{
			if (Candidate.GroupId == Rule.GroupId)
			{
				GroupCandidates.Add(Candidate);
			}
		}

		if (GroupCandidates.IsEmpty())
		{
			AddWarning(Result,
				FString::Printf(TEXT("Village group '%s' has no eligible candidates."), *Rule.GroupId.ToString()),
				Rule.bRequired);
			continue;
		}

		FRandomStream RandomStream(MakeGroupSeed(RunSeed, Rule.GroupId));
		const float SpawnChance = FMath::Clamp(Rule.SpawnChance, 0.0f, 1.0f);
		if (!Rule.bRequired
			&& (SpawnChance <= 0.0f || (SpawnChance < 1.0f && RandomStream.FRand() >= SpawnChance)))
		{
			Result.SkippedGroupIds.Add(Rule.GroupId);
			continue;
		}

		int32 RequestedPickCount = Rule.PickCount;
		int32 RequiredMinimumPickCount = Rule.PickCount;
		if (Rule.bUsePickCountRange)
		{
			RequiredMinimumPickCount = Rule.MinPickCount;
			const int32 EffectiveMinPickCount = FMath::Min(Rule.MinPickCount, GroupCandidates.Num());
			const int32 EffectiveMaxPickCount = FMath::Min(Rule.MaxPickCount, GroupCandidates.Num());
			RequestedPickCount = EffectiveMinPickCount == EffectiveMaxPickCount
				? EffectiveMinPickCount
				: RandomStream.RandRange(EffectiveMinPickCount, EffectiveMaxPickCount);
		}

		const int32 PickCount = FMath::Min(RequestedPickCount, GroupCandidates.Num());
		if (Rule.bRequired && GroupCandidates.Num() < RequiredMinimumPickCount)
		{
			AddWarning(Result,
				FString::Printf(
					TEXT("Required village group '%s' requires at least %d slots but only %d are eligible."),
					*Rule.GroupId.ToString(),
					RequiredMinimumPickCount,
					GroupCandidates.Num()),
				true);
		}
		for (int32 PickIndex = 0; PickIndex < PickCount; ++PickIndex)
		{
			const int32 SelectedIndex = SelectWeightedIndex(GroupCandidates, RandomStream);
			if (!GroupCandidates.IsValidIndex(SelectedIndex))
			{
				AddWarning(Result,
					FString::Printf(TEXT("Village group '%s' could not select a weighted candidate."), *Rule.GroupId.ToString()),
					true);
				break;
			}

			Result.SelectedSlotIds.Add(GroupCandidates[SelectedIndex].SlotId);
			GroupCandidates.RemoveAt(SelectedIndex);
		}
	}

	TSet<FName> ReportedUnconfiguredGroups;
	for (const FGP_VillageCandidate& Candidate : ValidCandidates)
	{
		if (!ConfiguredGroups.Contains(Candidate.GroupId) && !ReportedUnconfiguredGroups.Contains(Candidate.GroupId))
		{
			AddWarning(Result,
				FString::Printf(TEXT("Village candidate group '%s' has no matching rule."), *Candidate.GroupId.ToString()),
				false);
			ReportedUnconfiguredGroups.Add(Candidate.GroupId);
		}
	}

	Result.SelectedSlotIds.Sort(FNameLexicalLess());
	Result.SkippedGroupIds.Sort(FNameLexicalLess());
	return Result;
}
