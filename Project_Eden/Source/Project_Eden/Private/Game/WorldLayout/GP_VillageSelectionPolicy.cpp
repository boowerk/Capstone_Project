#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"

#include "Misc/Crc.h"

namespace
{
	struct FGPVillageGroupRequirement
	{
		FName GroupId = NAME_None;
		int32 Count = 0;
	};

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

	bool CandidatesOverlap(const FGP_VillageCandidate& First, const FGP_VillageCandidate& Second)
	{
		return First.SlotId != Second.SlotId
			&& (First.OverlappingSlotIds.Contains(Second.SlotId)
				|| Second.OverlappingSlotIds.Contains(First.SlotId));
	}

	bool OverlapsAnySelected(
		const FGP_VillageCandidate& Candidate,
		const TArray<FGP_VillageCandidate>& SelectedCandidates)
	{
		for (const FGP_VillageCandidate& SelectedCandidate : SelectedCandidates)
		{
			if (CandidatesOverlap(Candidate, SelectedCandidate))
			{
				return true;
			}
		}
		return false;
	}

	bool CanSelectNonOverlappingCount(
		const TArray<FGP_VillageCandidate>& Candidates,
		int32 RequiredCount)
	{
		if (RequiredCount <= 0)
		{
			return true;
		}
		if (Candidates.Num() < RequiredCount)
		{
			return false;
		}

		const int32 LastUsefulStartIndex = Candidates.Num() - RequiredCount;
		for (int32 CandidateIndex = 0; CandidateIndex <= LastUsefulStartIndex; ++CandidateIndex)
		{
			const FGP_VillageCandidate& SelectedCandidate = Candidates[CandidateIndex];
			TArray<FGP_VillageCandidate> CompatibleTail;
			for (int32 TailIndex = CandidateIndex + 1; TailIndex < Candidates.Num(); ++TailIndex)
			{
				if (!CandidatesOverlap(SelectedCandidate, Candidates[TailIndex]))
				{
					CompatibleTail.Add(Candidates[TailIndex]);
				}
			}

			if (CanSelectNonOverlappingCount(CompatibleTail, RequiredCount - 1))
			{
				return true;
			}
		}

		return false;
	}

	TArray<FGP_VillageCandidate> GetCompatibleCandidatesAfterPick(
		const TArray<FGP_VillageCandidate>& Candidates,
		const FGP_VillageCandidate& SelectedCandidate)
	{
		TArray<FGP_VillageCandidate> CompatibleCandidates;
		for (const FGP_VillageCandidate& Candidate : Candidates)
		{
			if (Candidate.SlotId != SelectedCandidate.SlotId
				&& !CandidatesOverlap(SelectedCandidate, Candidate))
			{
				CompatibleCandidates.Add(Candidate);
			}
		}
		return CompatibleCandidates;
	}

	bool ContainsSelectedSlot(
		const TArray<FGP_VillageCandidate>& SelectedCandidates,
		FName SlotId)
	{
		for (const FGP_VillageCandidate& SelectedCandidate : SelectedCandidates)
		{
			if (SelectedCandidate.SlotId == SlotId)
			{
				return true;
			}
		}
		return false;
	}

	bool CanSatisfyGroupRequirements(
		const TArray<FGP_VillageCandidate>& Candidates,
		const TArray<FGP_VillageCandidate>& SelectedCandidates,
		const TArray<FGPVillageGroupRequirement>& Requirements,
		int32 RequirementIndex = 0)
	{
		while (Requirements.IsValidIndex(RequirementIndex)
			&& Requirements[RequirementIndex].Count <= 0)
		{
			++RequirementIndex;
		}
		if (!Requirements.IsValidIndex(RequirementIndex))
		{
			return true;
		}

		const FGPVillageGroupRequirement& Requirement = Requirements[RequirementIndex];
		for (const FGP_VillageCandidate& Candidate : Candidates)
		{
			if (Candidate.GroupId != Requirement.GroupId
				|| ContainsSelectedSlot(SelectedCandidates, Candidate.SlotId)
				|| OverlapsAnySelected(Candidate, SelectedCandidates))
			{
				continue;
			}

			TArray<FGP_VillageCandidate> ProposedSelection = SelectedCandidates;
			ProposedSelection.Add(Candidate);
			TArray<FGPVillageGroupRequirement> RemainingRequirements = Requirements;
			--RemainingRequirements[RequirementIndex].Count;
			if (CanSatisfyGroupRequirements(
				Candidates,
				ProposedSelection,
				RemainingRequirements,
				RequirementIndex))
			{
				return true;
			}
		}

		return false;
	}

	TArray<FGPVillageGroupRequirement> BuildPendingRequiredGroups(
		const TArray<FGPVillageGroupRequirement>& RequiredGroupMinimums,
		FName CurrentGroupId,
		int32 CurrentGroupCount)
	{
		TArray<FGPVillageGroupRequirement> PendingRequirements;
		bool bReachedCurrentGroup = false;
		for (const FGPVillageGroupRequirement& Requirement : RequiredGroupMinimums)
		{
			if (Requirement.GroupId == CurrentGroupId)
			{
				bReachedCurrentGroup = true;
				if (CurrentGroupCount > 0)
				{
					PendingRequirements.Add({Requirement.GroupId, CurrentGroupCount});
				}
				continue;
			}

			if (bReachedCurrentGroup)
			{
				PendingRequirements.Add(Requirement);
			}
		}
		return PendingRequirements;
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
		if (A.bRequired != B.bRequired)
		{
			return A.bRequired;
		}
		return A.GroupId.LexicalLess(B.GroupId);
	});

	TArray<FGPVillageGroupRequirement> RequiredGroupMinimums;
	for (const FGP_VillageGroupRule& Rule : SortedRules)
	{
		const bool bHasValidCount = Rule.bUsePickCountRange
			? Rule.MinPickCount >= 1 && Rule.MaxPickCount >= Rule.MinPickCount
			: Rule.PickCount > 0;
		if (Rule.bRequired
			&& !Rule.GroupId.IsNone()
			&& GroupRuleCounts.FindRef(Rule.GroupId) == 1
			&& bHasValidCount)
		{
			RequiredGroupMinimums.Add({
				Rule.GroupId,
				Rule.bUsePickCountRange ? Rule.MinPickCount : Rule.PickCount});
		}
	}

	TSet<FName> ConfiguredGroups;
	TArray<FGP_VillageCandidate> SelectedCandidates;
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
			if (Candidate.GroupId == Rule.GroupId
				&& !OverlapsAnySelected(Candidate, SelectedCandidates))
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

		int32 LocalTargetPickCount = FMath::Min(RequestedPickCount, GroupCandidates.Num());
		while (LocalTargetPickCount > 0
			&& !CanSelectNonOverlappingCount(GroupCandidates, LocalTargetPickCount))
		{
			--LocalTargetPickCount;
		}

		int32 TargetPickCount = LocalTargetPickCount;
		bool bUseGlobalRequiredFeasibility = false;
		if (Rule.bRequired && LocalTargetPickCount >= RequiredMinimumPickCount)
		{
			int32 GlobalTargetPickCount = LocalTargetPickCount;
			while (GlobalTargetPickCount >= RequiredMinimumPickCount
				&& !CanSatisfyGroupRequirements(
					ValidCandidates,
					SelectedCandidates,
					BuildPendingRequiredGroups(
						RequiredGroupMinimums,
						Rule.GroupId,
						GlobalTargetPickCount)))
			{
				--GlobalTargetPickCount;
			}

			if (GlobalTargetPickCount >= RequiredMinimumPickCount)
			{
				TargetPickCount = GlobalTargetPickCount;
				bUseGlobalRequiredFeasibility = true;
			}
		}

		int32 SelectedInGroup = 0;
		for (int32 PickIndex = 0;
			PickIndex < TargetPickCount && !GroupCandidates.IsEmpty();
			++PickIndex)
		{
			const int32 RemainingPickCount = TargetPickCount - PickIndex - 1;
			TArray<FGP_VillageCandidate> FeasibleCandidates;
			for (const FGP_VillageCandidate& Candidate : GroupCandidates)
			{
				bool bCandidateIsFeasible = false;
				if (bUseGlobalRequiredFeasibility)
				{
					TArray<FGP_VillageCandidate> ProposedSelection = SelectedCandidates;
					ProposedSelection.Add(Candidate);
					bCandidateIsFeasible = CanSatisfyGroupRequirements(
						ValidCandidates,
						ProposedSelection,
						BuildPendingRequiredGroups(
							RequiredGroupMinimums,
							Rule.GroupId,
							RemainingPickCount));
				}
				else
				{
					const TArray<FGP_VillageCandidate> CompatibleCandidates =
						GetCompatibleCandidatesAfterPick(GroupCandidates, Candidate);
					bCandidateIsFeasible =
						CanSelectNonOverlappingCount(CompatibleCandidates, RemainingPickCount);
				}

				if (bCandidateIsFeasible)
				{
					FeasibleCandidates.Add(Candidate);
				}
			}

			const int32 SelectedIndex = SelectWeightedIndex(FeasibleCandidates, RandomStream);
			if (!FeasibleCandidates.IsValidIndex(SelectedIndex))
			{
				AddWarning(Result,
					FString::Printf(TEXT("Village group '%s' could not select a weighted candidate."), *Rule.GroupId.ToString()),
					true);
				break;
			}

			const FGP_VillageCandidate SelectedCandidate = FeasibleCandidates[SelectedIndex];
			Result.SelectedSlotIds.Add(SelectedCandidate.SlotId);
			SelectedCandidates.Add(SelectedCandidate);
			++SelectedInGroup;
			GroupCandidates.RemoveAll([&SelectedCandidate](const FGP_VillageCandidate& Candidate)
			{
				return Candidate.SlotId == SelectedCandidate.SlotId
					|| CandidatesOverlap(SelectedCandidate, Candidate);
			});
		}

		if (SelectedInGroup < RequiredMinimumPickCount && Rule.bRequired)
		{
			AddWarning(Result,
				FString::Printf(
					TEXT("Required village group '%s' requires at least %d non-overlapping slots but only %d could be selected."),
					*Rule.GroupId.ToString(),
					RequiredMinimumPickCount,
					SelectedInGroup),
				true);
		}
		else if (SelectedInGroup < RequestedPickCount)
		{
			AddWarning(Result,
				FString::Printf(
					TEXT("Village group '%s' requested %d slots but only %d non-overlapping slots could be selected."),
					*Rule.GroupId.ToString(),
					RequestedPickCount,
					SelectedInGroup),
				false);
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
