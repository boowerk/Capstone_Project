#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/BoxComponent.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/AutomationTest.h"

namespace
{
	FGP_VillageCandidate MakeCandidate(FName SlotId, FName GroupId, float Weight = 1.0f)
	{
		FGP_VillageCandidate Candidate;
		Candidate.SlotId = SlotId;
		Candidate.GroupId = GroupId;
		Candidate.SelectionWeight = Weight;
		return Candidate;
	}

	FGP_VillageGroupRule MakeRule(FName GroupId, bool bRequired, float SpawnChance, int32 PickCount = 1)
	{
		FGP_VillageGroupRule Rule;
		Rule.GroupId = GroupId;
		Rule.bRequired = bRequired;
		Rule.SpawnChance = SpawnChance;
		Rule.PickCount = PickCount;
		return Rule;
	}

	FName FindSelectedInGroup(
		const FGP_VillageSelectionResult& Result,
		const TArray<FGP_VillageCandidate>& Candidates,
		FName GroupId)
	{
		for (FName SelectedSlotId : Result.SelectedSlotIds)
		{
			for (const FGP_VillageCandidate& Candidate : Candidates)
			{
				if (Candidate.SlotId == SelectedSlotId && Candidate.GroupId == GroupId)
				{
					return SelectedSlotId;
				}
			}
		}
		return NAME_None;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVillageSelectionPolicyTest,
	"ProjectEden.Game.WorldLayout.VillageSelection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVillageSelectionPolicyTest::RunTest(const FString& Parameters)
{
	const TArray<FGP_VillageCandidate> MainCandidates = {
		MakeCandidate(TEXT("Main_A"), TEXT("Main")),
		MakeCandidate(TEXT("Main_B"), TEXT("Main")),
		MakeCandidate(TEXT("Main_C"), TEXT("Main"))
	};

	const TArray<FGP_VillageGroupRule> RequiredRule = {
		MakeRule(TEXT("Main"), true, 0.0f)
	};

	const FGP_VillageSelectionResult RequiredResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, RequiredRule);
	TestTrue(TEXT("Required group selection succeeds"), RequiredResult.bSucceeded);
	TestEqual(TEXT("Required group selects exactly one slot"), RequiredResult.SelectedSlotIds.Num(), 1);

	const TArray<FGP_VillageGroupRule> NeverSpawnRule = {
		MakeRule(TEXT("Main"), false, 0.0f)
	};
	const FGP_VillageSelectionResult NeverSpawnResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, NeverSpawnRule);
	TestTrue(TEXT("Optional zero-chance group is a valid result"), NeverSpawnResult.bSucceeded);
	TestTrue(TEXT("Optional zero-chance group selects no village"), NeverSpawnResult.SelectedSlotIds.IsEmpty());
	TestTrue(TEXT("Optional zero-chance group is reported as skipped"), NeverSpawnResult.SkippedGroupIds.Contains(TEXT("Main")));

	const TArray<FGP_VillageGroupRule> PickTwoRule = {
		MakeRule(TEXT("Main"), false, 1.0f, 2)
	};
	const FGP_VillageSelectionResult PickTwoResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, PickTwoRule);
	TestEqual(TEXT("PickCount selects the requested number without replacement"), PickTwoResult.SelectedSlotIds.Num(), 2);
	if (PickTwoResult.SelectedSlotIds.Num() == 2)
	{
		TestNotEqual(TEXT("Selected slots are unique"), PickTwoResult.SelectedSlotIds[0], PickTwoResult.SelectedSlotIds[1]);
	}

	TArray<FGP_VillageCandidate> ShuffledCandidates = MainCandidates;
	Algo::Reverse(ShuffledCandidates);
	const FGP_VillageSelectionResult ShuffledResult =
		GPVillageSelectionPolicy::SelectSlots(1337, ShuffledCandidates, RequiredRule);
	TestTrue(TEXT("Candidate enumeration order does not change the result"),
		RequiredResult.SelectedSlotIds == ShuffledResult.SelectedSlotIds);

	TArray<FGP_VillageCandidate> WithOtherGroup = MainCandidates;
	WithOtherGroup.Add(MakeCandidate(TEXT("Other_A"), TEXT("Other")));
	WithOtherGroup.Add(MakeCandidate(TEXT("Other_B"), TEXT("Other")));
	TArray<FGP_VillageGroupRule> WithOtherRules = RequiredRule;
	WithOtherRules.Add(MakeRule(TEXT("Other"), true, 1.0f));
	const FGP_VillageSelectionResult WithOtherResult =
		GPVillageSelectionPolicy::SelectSlots(1337, WithOtherGroup, WithOtherRules);
	TestEqual(TEXT("Adding another group does not perturb the existing group"),
		FindSelectedInGroup(WithOtherResult, WithOtherGroup, TEXT("Main")),
		RequiredResult.SelectedSlotIds[0]);

	const TArray<FGP_VillageCandidate> WeightedCandidates = {
		MakeCandidate(TEXT("Disabled"), TEXT("Weighted"), 0.0f),
		MakeCandidate(TEXT("Enabled"), TEXT("Weighted"), 1.0f)
	};
	const TArray<FGP_VillageGroupRule> WeightedRule = {
		MakeRule(TEXT("Weighted"), true, 1.0f)
	};
	const FGP_VillageSelectionResult WeightedResult =
		GPVillageSelectionPolicy::SelectSlots(99, WeightedCandidates, WeightedRule);
	TestEqual(TEXT("Weighted group selects one slot"), WeightedResult.SelectedSlotIds.Num(), 1);
	if (WeightedResult.SelectedSlotIds.Num() == 1)
	{
		TestEqual(TEXT("Zero-weight candidates are never selected"), WeightedResult.SelectedSlotIds[0], FName(TEXT("Enabled")));
	}

	const TArray<FGP_VillageCandidate> DuplicateCandidates = {
		MakeCandidate(TEXT("Duplicate"), TEXT("Main")),
		MakeCandidate(TEXT("Duplicate"), TEXT("Main"))
	};
	const FGP_VillageSelectionResult DuplicateResult =
		GPVillageSelectionPolicy::SelectSlots(1337, DuplicateCandidates, RequiredRule);
	TestFalse(TEXT("Duplicate SlotIds fail closed"), DuplicateResult.bSucceeded);
	TestTrue(TEXT("Duplicate SlotIds are not selected"), DuplicateResult.SelectedSlotIds.IsEmpty());

	const TArray<FGP_VillageGroupRule> TooManyRequiredRule = {
		MakeRule(TEXT("Main"), true, 1.0f, 4)
	};
	const FGP_VillageSelectionResult TooManyRequiredResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, TooManyRequiredRule);
	TestFalse(TEXT("Required group reports insufficient candidates"), TooManyRequiredResult.bSucceeded);
	TestEqual(TEXT("Required group still selects every eligible fallback"), TooManyRequiredResult.SelectedSlotIds.Num(), 3);

	TestNotNull(TEXT("VillageSlot owns an editor footprint box"), GetDefault<AGP_VillageSlot>()->GetSlotBounds());
	return true;
}

#endif
