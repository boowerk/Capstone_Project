#if WITH_DEV_AUTOMATION_TESTS

#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_CrystalCorruptionRegionEventActor.h"
#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/RegionEvents/GP_RegionEventSelectionPolicy.h"
#include "Game/RegionEvents/GP_ShrineRuinsRegionEventActor.h"
#include "Game/RegionEvents/GP_StructureDefenseRegionEventActor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionEventSelectionPolicyTest,
	"ProjectEden.Game.RegionEvents.Selection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionEventSelectionPolicyTest::RunTest(const FString& Parameters)
{
	UGP_RegionEventData* AmbushEvent = NewObject<UGP_RegionEventData>();
	AmbushEvent->EventId = TEXT("Ambush");
	AmbushEvent->Trigger = EGPRegionEventTrigger::ZoneStarted;
	AmbushEvent->SelectionWeight = 2.0f;

	UGP_RegionEventData* RewardEvent = NewObject<UGP_RegionEventData>();
	RewardEvent->EventId = TEXT("Reward");
	RewardEvent->Trigger = EGPRegionEventTrigger::ZoneCompleted;
	RewardEvent->SelectionWeight = 5.0f;

	UGP_RegionEventData* DisabledEvent = NewObject<UGP_RegionEventData>();
	DisabledEvent->EventId = TEXT("Disabled");
	DisabledEvent->Trigger = EGPRegionEventTrigger::ZoneStarted;
	DisabledEvent->SelectionWeight = 0.0f;

	TArray<TObjectPtr<UGP_RegionEventData>> EventPool;
	EventPool.Add(AmbushEvent);
	EventPool.Add(RewardEvent);
	EventPool.Add(DisabledEvent);

	TestEqual(TEXT("Only matching positive-weight ZoneStarted events contribute weight"),
		GPRegionEventSelectionPolicy::GetTotalWeight(EventPool, EGPRegionEventTrigger::ZoneStarted),
		2.0f);
	TestEqual(TEXT("Only matching positive-weight ZoneCompleted events contribute weight"),
		GPRegionEventSelectionPolicy::GetTotalWeight(EventPool, EGPRegionEventTrigger::ZoneCompleted),
		5.0f);

	FRandomStream RandomStream(1337);
	for (int32 RollIndex = 0; RollIndex < 16; ++RollIndex)
	{
		const UGP_RegionEventData* SelectedEvent = GPRegionEventSelectionPolicy::SelectWeightedEvent(
			EventPool,
			EGPRegionEventTrigger::ZoneStarted,
			RandomStream);
		TestTrue(TEXT("ZoneStarted roll selects the enabled ambush event only"), SelectedEvent == AmbushEvent);
	}

	DisabledEvent->SelectionWeight = 0.0f;
	AmbushEvent->SelectionWeight = 0.0f;
	TestNull(TEXT("No positive matching weight returns no event"),
		GPRegionEventSelectionPolicy::SelectWeightedEvent(EventPool, EGPRegionEventTrigger::ZoneStarted, RandomStream));

	TestEqual(TEXT("Event actors start dormant before the director activates them"),
		GetDefault<AGP_RegionEventActor>()->GetRuntimeState(),
		EGPRegionEventRuntimeState::Dormant);
	TestEqual(TEXT("The director has no active events by default"),
		GetDefault<AGP_RegionEventDirector>()->GetActiveEventCount(),
		0);

	AmbushEvent->EventActorClass = AGP_RedRiftRegionEventActor::StaticClass();
	TestTrue(TEXT("Region event data can override the runtime actor class"),
		AmbushEvent->EventActorClass->IsChildOf(AGP_RegionEventActor::StaticClass()));
	TestTrue(TEXT("Crystal corruption event is a region event actor"),
		AGP_CrystalCorruptionRegionEventActor::StaticClass()->IsChildOf(AGP_RegionEventActor::StaticClass()));
	TestTrue(TEXT("Shrine ruins event is a region event actor"),
		AGP_ShrineRuinsRegionEventActor::StaticClass()->IsChildOf(AGP_RegionEventActor::StaticClass()));
	TestTrue(TEXT("Structure defense event is a region event actor"),
		AGP_StructureDefenseRegionEventActor::StaticClass()->IsChildOf(AGP_RegionEventActor::StaticClass()));

	return true;
}

#endif
