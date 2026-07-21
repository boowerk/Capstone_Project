#if WITH_DEV_AUTOMATION_TESTS

#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "Game/RegionEvents/GP_RegionEventExplorationPolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPRegionEventExplorationPolicyTest,
	"ProjectEden.Game.RegionEvents.ExplorationPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPRegionEventExplorationPolicyTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Attempt chance uses local corruption"),
		GPRegionEventExplorationPolicy::CalculateAttemptChance(0.5f, 0.30f, 0.40f),
		0.50f);
	TestEqual(TEXT("Attempt chance is clamped"),
		GPRegionEventExplorationPolicy::CalculateAttemptChance(1.0f, 0.90f, 0.90f),
		1.0f);

	UGP_RegionEventData* Shrine = NewObject<UGP_RegionEventData>();
	Shrine->EventId = TEXT("Shrine");
	Shrine->EventType = EGPRegionEventType::ShrineCache;
	Shrine->EventActorClass = AGP_RegionEventActor::StaticClass();
	Shrine->SelectionWeight = 1.0f;

	UGP_RegionEventData* Rift = NewObject<UGP_RegionEventData>();
	Rift->EventId = TEXT("Rift");
	Rift->EventType = EGPRegionEventType::CorruptionRift;
	Rift->EventActorClass = AGP_RegionEventActor::StaticClass();
	Rift->SelectionWeight = 1.0f;

	TestTrue(TEXT("Clean regions favour shrine rewards"),
		GPRegionEventExplorationPolicy::CalculateCorruptionWeight(*Shrine, 0.0f)
			> GPRegionEventExplorationPolicy::CalculateCorruptionWeight(*Rift, 0.0f));
	TestTrue(TEXT("Corrupted regions favour hostile rifts"),
		GPRegionEventExplorationPolicy::CalculateCorruptionWeight(*Rift, 1.0f)
			> GPRegionEventExplorationPolicy::CalculateCorruptionWeight(*Shrine, 1.0f));

	TArray<TObjectPtr<UGP_RegionEventData>> Pool = { Shrine, Rift };
	FRandomStream RandomStream(142);
	const UGP_RegionEventData* SelectedWithoutRepeat =
		GPRegionEventExplorationPolicy::SelectWeightedExplorationEvent(Pool, 0.5f, Shrine->EventId, RandomStream);
	TestTrue(TEXT("The previous event is excluded when an alternative exists"), SelectedWithoutRepeat == Rift);

	Rift->MinimumCorruptionNormalized = 0.75f;
	RandomStream.Initialize(143);
	const UGP_RegionEventData* SelectedBelowThreshold =
		GPRegionEventExplorationPolicy::SelectWeightedExplorationEvent(Pool, 0.5f, NAME_None, RandomStream);
	TestTrue(TEXT("Corruption thresholds remove ineligible events"), SelectedBelowThreshold == Shrine);

	const AGP_RegionEventDirector* DirectorDefaults = GetDefault<AGP_RegionEventDirector>();
	TestNotNull(TEXT("Region event director defaults are available"), DirectorDefaults);
	if (IsValid(DirectorDefaults))
	{
		// L_LandscapeMap's placed BP inherits this native pool without replacing the user's legacy EventPool.
		TestEqual(TEXT("Four production exploration definitions are loaded by default"),
			DirectorDefaults->GetExplorationEventPoolCount(),
			4);
	}

	return true;
}

#endif
