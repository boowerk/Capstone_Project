#if WITH_DEV_AUTOMATION_TESTS

#include "Algo/Reverse.h"
#include "Components/BoxComponent.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"
#include "Game/WorldLayout/GP_VillageSelectionPolicy.h"
#include "Game/WorldLayout/GP_VillageSlot.h"
#include "Misc/AutomationTest.h"
#include "Elements/PCGActorSelector.h"
#include "Elements/PCGDataFromActor.h"
#include "Elements/PCGReroute.h"
#include "Elements/PCGUserParameterGet.h"
#include "PCGEdge.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"

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

	FGP_VillageGroupRule MakeRangeRule(
		FName GroupId,
		bool bRequired,
		float SpawnChance,
		int32 MinPickCount,
		int32 MaxPickCount)
	{
		FGP_VillageGroupRule Rule = MakeRule(GroupId, bRequired, SpawnChance);
		Rule.bUsePickCountRange = true;
		Rule.MinPickCount = MinPickCount;
		Rule.MaxPickCount = MaxPickCount;
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

	const FPCGSettingsOverridableParam* FindActorTagOverride(
		const UPCGDataFromActorSettings* Settings)
	{
		return Settings ? Settings->OverridableParams().FindByPredicate(
			[](const FPCGSettingsOverridableParam& Param)
			{
				return Param.PropertiesNames.Num() == 2
					&& Param.PropertiesNames[0]
						== GET_MEMBER_NAME_CHECKED(UPCGDataFromActorSettings, ActorSelector)
					&& Param.PropertiesNames[1]
						== GET_MEMBER_NAME_CHECKED(FPCGActorSelectorSettings, ActorSelectionTag);
			}) : nullptr;
	}

	bool IsDrivenByGraphParameter(
		const UPCGPin* TargetInput,
		const FPropertyBagPropertyDesc& ParameterDesc)
	{
		TArray<const UPCGPin*> PendingPins;
		TSet<const UPCGPin*> VisitedPins;
		PendingPins.Add(TargetInput);

		while (!PendingPins.IsEmpty())
		{
			const UPCGPin* DownstreamPin = PendingPins.Pop(EAllowShrinking::No);
			if (!DownstreamPin || VisitedPins.Contains(DownstreamPin))
			{
				continue;
			}

			VisitedPins.Add(DownstreamPin);
			for (const UPCGEdge* Edge : DownstreamPin->Edges)
			{
				if (!Edge || Edge->OutputPin != DownstreamPin)
				{
					continue;
				}

				const UPCGPin* UpstreamPin = Edge->InputPin;
				const UPCGNode* UpstreamNode = UpstreamPin ? UpstreamPin->Node.Get() : nullptr;
				if (!UpstreamNode)
				{
					continue;
				}

				if (const UPCGUserParameterGetSettings* Getter =
					Cast<UPCGUserParameterGetSettings>(UpstreamNode->GetSettings()))
				{
					if (Getter->PropertyGuid == ParameterDesc.ID
						&& Getter->PropertyName == ParameterDesc.Name
						&& UpstreamPin->Properties.Label == ParameterDesc.Name)
					{
						return true;
					}
				}

				if (Cast<UPCGRerouteSettings>(UpstreamNode->GetSettings()))
				{
					for (const UPCGPin* RerouteInput : UpstreamNode->GetInputPins())
					{
						PendingPins.Add(RerouteInput);
					}
				}
			}
		}

		return false;
	}

	int32 CountActorTagSelectorsDrivenByParameter(
		const UPCGGraph* Graph,
		const FPropertyBagPropertyDesc& ParameterDesc)
	{
		int32 ConnectedSelectorCount = 0;
		for (const UPCGNode* Node : Graph->GetNodes())
		{
			const UPCGDataFromActorSettings* Settings =
				Node ? Cast<UPCGDataFromActorSettings>(Node->GetSettings()) : nullptr;
			if (!Settings
				|| Settings->ActorSelector.ActorFilter != EPCGActorFilter::AllWorldActors
				|| Settings->ActorSelector.ActorSelection != EPCGActorSelection::ByTag)
			{
				continue;
			}

			const FPCGSettingsOverridableParam* Override = FindActorTagOverride(Settings);
			const UPCGPin* TagInput = Override ? Node->GetInputPin(Override->Label) : nullptr;
			if (TagInput && IsDrivenByGraphParameter(TagInput, ParameterDesc))
			{
				++ConnectedSelectorCount;
			}
		}

		return ConnectedSelectorCount;
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
	const TArray<FGP_VillageGroupRule> FixedRangeTwoRule = {
		MakeRangeRule(TEXT("Main"), false, 1.0f, 2, 2)
	};
	TArray<FGP_VillageGroupRule> FixedRangeIgnoresLegacyCountRule = FixedRangeTwoRule;
	FixedRangeIgnoresLegacyCountRule[0].PickCount = 99;
	const FGP_VillageSelectionResult FixedRangeTwoResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, FixedRangeIgnoresLegacyCountRule);
	TestTrue(TEXT("Equal range bounds preserve fixed-count selection"),
		FixedRangeTwoResult.SelectedSlotIds == PickTwoResult.SelectedSlotIds);

	const TArray<FGP_VillageCandidate> RangeCandidates = {
		MakeCandidate(TEXT("Range_A"), TEXT("Range")),
		MakeCandidate(TEXT("Range_B"), TEXT("Range")),
		MakeCandidate(TEXT("Range_C"), TEXT("Range")),
		MakeCandidate(TEXT("Range_D"), TEXT("Range")),
		MakeCandidate(TEXT("Range_E"), TEXT("Range")),
		MakeCandidate(TEXT("Range_F"), TEXT("Range")),
		MakeCandidate(TEXT("Range_G"), TEXT("Range")),
		MakeCandidate(TEXT("Range_H"), TEXT("Range"))
	};
	const TArray<FGP_VillageGroupRule> RangeRule = {
		MakeRangeRule(TEXT("Range"), true, 1.0f, 2, 5)
	};
	bool bAllRangeSelectionsSucceeded = true;
	bool bAllCountsWithinRange = true;
	TSet<int32> ObservedRangeCounts;
	for (int32 Seed = 0; Seed < 64; ++Seed)
	{
		const FGP_VillageSelectionResult RangeResult =
			GPVillageSelectionPolicy::SelectSlots(Seed, RangeCandidates, RangeRule);
		bAllRangeSelectionsSucceeded &= RangeResult.bSucceeded;
		bAllCountsWithinRange &= RangeResult.SelectedSlotIds.Num() >= 2
			&& RangeResult.SelectedSlotIds.Num() <= 5;
		ObservedRangeCounts.Add(RangeResult.SelectedSlotIds.Num());
	}
	TestTrue(TEXT("Pick-count range succeeds across run seeds"), bAllRangeSelectionsSucceeded);
	TestTrue(TEXT("Pick-count range stays within its authored bounds"), bAllCountsWithinRange);
	TestTrue(TEXT("Pick-count range varies across run seeds"), ObservedRangeCounts.Num() > 1);

	const FGP_VillageSelectionResult DeterministicRangeResult =
		GPVillageSelectionPolicy::SelectSlots(4242, RangeCandidates, RangeRule);
	const FGP_VillageSelectionResult RepeatedRangeResult =
		GPVillageSelectionPolicy::SelectSlots(4242, RangeCandidates, RangeRule);
	TestTrue(TEXT("Pick-count range is deterministic for the same run seed"),
		DeterministicRangeResult.SelectedSlotIds == RepeatedRangeResult.SelectedSlotIds);

	TArray<FGP_VillageCandidate> ShuffledRangeCandidates = RangeCandidates;
	Algo::Reverse(ShuffledRangeCandidates);
	const FGP_VillageSelectionResult ShuffledRangeResult =
		GPVillageSelectionPolicy::SelectSlots(4242, ShuffledRangeCandidates, RangeRule);
	TestTrue(TEXT("Pick-count range is independent of candidate enumeration order"),
		DeterministicRangeResult.SelectedSlotIds == ShuffledRangeResult.SelectedSlotIds);

	const TArray<FGP_VillageGroupRule> InvalidRangeRule = {
		MakeRangeRule(TEXT("Main"), true, 1.0f, 3, 2)
	};
	const FGP_VillageSelectionResult InvalidRangeResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, InvalidRangeRule);
	TestFalse(TEXT("Invalid pick-count range fails closed"), InvalidRangeResult.bSucceeded);
	TestTrue(TEXT("Invalid pick-count range selects no slots"), InvalidRangeResult.SelectedSlotIds.IsEmpty());
	const TArray<FGP_VillageGroupRule> ZeroMinimumRangeRule = {
		MakeRangeRule(TEXT("Main"), false, 1.0f, 0, 2)
	};
	const FGP_VillageSelectionResult ZeroMinimumRangeResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, ZeroMinimumRangeRule);
	TestFalse(TEXT("Pick-count range minimum must be positive"), ZeroMinimumRangeResult.bSucceeded);

	const TArray<FGP_VillageGroupRule> TooManyRequiredRangeRule = {
		MakeRangeRule(TEXT("Main"), true, 1.0f, 4, 8)
	};
	const FGP_VillageSelectionResult TooManyRequiredRangeResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, TooManyRequiredRangeRule);
	TestFalse(TEXT("Required range reports an unsatisfied minimum"), TooManyRequiredRangeResult.bSucceeded);
	TestEqual(TEXT("Required range still selects every eligible fallback"),
		TooManyRequiredRangeResult.SelectedSlotIds.Num(),
		MainCandidates.Num());

	const TArray<FGP_VillageGroupRule> OptionalRangeAboveCandidatesRule = {
		MakeRangeRule(TEXT("Main"), false, 1.0f, 4, 8)
	};
	const FGP_VillageSelectionResult OptionalRangeAboveCandidatesResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, OptionalRangeAboveCandidatesRule);
	TestTrue(TEXT("Optional range clamps to available candidates"),
		OptionalRangeAboveCandidatesResult.bSucceeded);
	TestEqual(TEXT("Optional range selects every eligible fallback"),
		OptionalRangeAboveCandidatesResult.SelectedSlotIds.Num(),
		MainCandidates.Num());

	TArray<FGP_VillageGroupRule> FixedRuleWithInvalidInactiveRange = PickTwoRule;
	FixedRuleWithInvalidInactiveRange[0].MinPickCount = 9;
	FixedRuleWithInvalidInactiveRange[0].MaxPickCount = 1;
	const FGP_VillageSelectionResult FixedInvalidInactiveRangeResult =
		GPVillageSelectionPolicy::SelectSlots(1337, MainCandidates, FixedRuleWithInvalidInactiveRange);
	TestTrue(TEXT("Fixed mode ignores inactive range values"),
		FixedInvalidInactiveRangeResult.SelectedSlotIds == PickTwoResult.SelectedSlotIds);

	TArray<FGP_VillageCandidate> ShuffledCandidates = MainCandidates;
	Algo::Reverse(ShuffledCandidates);
	const FGP_VillageSelectionResult ShuffledResult =
		GPVillageSelectionPolicy::SelectSlots(1337, ShuffledCandidates, RequiredRule);
	TestTrue(TEXT("Candidate enumeration order does not change the result"),
		RequiredResult.SelectedSlotIds == ShuffledResult.SelectedSlotIds);
	const FGP_VillageSelectionResult ShuffledPickTwoResult =
		GPVillageSelectionPolicy::SelectSlots(1337, ShuffledCandidates, PickTwoRule);
	TestTrue(TEXT("Multi-slot selection is independent of candidate enumeration order"),
		PickTwoResult.SelectedSlotIds == ShuffledPickTwoResult.SelectedSlotIds);

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
	TestNull(TEXT("VillageSlot Data Layer is explicitly authored per placed slot"),
		GetDefault<AGP_VillageSlot>()->GetVillageDataLayer());
	TestNotNull(TEXT("VillageSlot exposes a VillageDataLayer property"),
		AGP_VillageSlot::StaticClass()->FindPropertyByName(TEXT("VillageDataLayer")));
	TestNotNull(TEXT("VillageLayoutDirector exposes a single VillageLevelPreset property"),
		AGP_VillageLayoutDirector::StaticClass()->FindPropertyByName(TEXT("VillageLevelPreset")));
	TestNotNull(TEXT("Village group rules expose optional range mode"),
		FGP_VillageGroupRule::StaticStruct()->FindPropertyByName(TEXT("bUsePickCountRange")));
	TestNotNull(TEXT("Village group rules expose minimum pick count"),
		FGP_VillageGroupRule::StaticStruct()->FindPropertyByName(TEXT("MinPickCount")));
	TestNotNull(TEXT("Village group rules expose maximum pick count"),
		FGP_VillageGroupRule::StaticStruct()->FindPropertyByName(TEXT("MaxPickCount")));
	const FGP_VillageGroupRule DefaultGroupRule;
	TestFalse(TEXT("Village group rules default to legacy fixed-count mode"),
		DefaultGroupRule.bUsePickCountRange);
	TestEqual(TEXT("Village range minimum defaults to one"), DefaultGroupRule.MinPickCount, 1);
	TestEqual(TEXT("Village range maximum defaults to one"), DefaultGroupRule.MaxPickCount, 1);
	TestEqual(TEXT("VillageLayoutDirector defaults to the first authored village preset"),
		GetDefault<AGP_VillageLayoutDirector>()->GetVillageLevelPreset().ToSoftObjectPath(),
		FSoftObjectPath(TEXT("/Game/WorldLayout/L_Village_00.L_Village_00")));

	UPCGGraph* CityGraph = LoadObject<UPCGGraph>(
		nullptr,
		TEXT("/Game/RegionSystem/PCG/CityGen/PCG_CityGen_FromAnchor.PCG_CityGen_FromAnchor"));
	TestNotNull(TEXT("City PCG graph is available for village streaming"), CityGraph);
	if (CityGraph)
	{
		const TValueOrError<FName, EPropertyBagResult> RoadTag =
			CityGraph->GetGraphParameter<FName>(TEXT("RoadTag"));
		const TValueOrError<FName, EPropertyBagResult> DistrictTag =
			CityGraph->GetGraphParameter<FName>(TEXT("DistrictTag"));
		TestFalse(TEXT("City PCG exposes a RoadTag name parameter"), RoadTag.HasError());
		TestFalse(TEXT("City PCG exposes a DistrictTag name parameter"), DistrictTag.HasError());
		if (!RoadTag.HasError())
		{
			TestEqual(TEXT("City PCG road source tag is authored"), RoadTag.GetValue(), FName(TEXT("City_00_Road")));
		}
		if (!DistrictTag.HasError())
		{
			TestEqual(
				TEXT("City PCG district source tag is authored"),
				DistrictTag.GetValue(),
				FName(TEXT("City_00_District")));
		}

		const FInstancedPropertyBag* UserParameters = CityGraph->GetUserParametersStruct();
		TestNotNull(TEXT("City PCG user parameter bag is available"), UserParameters);
		if (UserParameters)
		{
			for (const FName ParameterName : {FName(TEXT("RoadTag")), FName(TEXT("DistrictTag"))})
			{
				const FPropertyBagPropertyDesc* ParameterDesc =
					UserParameters->FindPropertyDescByName(ParameterName);
				TestNotNull(
					*FString::Printf(TEXT("%s descriptor is available"), *ParameterName.ToString()),
					ParameterDesc);
				if (ParameterDesc)
				{
					TestTrue(
						*FString::Printf(
							TEXT("%s drives a tagged Get Actor Data selector"),
							*ParameterName.ToString()),
						CountActorTagSelectorsDrivenByParameter(CityGraph, *ParameterDesc) > 0);
				}
			}
		}
	}
	return true;
}

#endif
