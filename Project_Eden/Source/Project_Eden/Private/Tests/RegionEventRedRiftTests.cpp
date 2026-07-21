#if WITH_DEV_AUTOMATION_TESTS

#include "Game/RegionEvents/GP_RedRiftRegionEventActor.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRegionEventRedRiftCompletionPolicyTest,
	"ProjectEden.Game.RegionEvents.RedRiftCompletionPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRegionEventRedRiftCompletionPolicyTest::RunTest(const FString& Parameters)
{
	// A finite rift succeeds only after both halves of the contract are true: final wave spawned and all owned enemies dead.
	TestTrue(TEXT("Final cleared wave completes"),
		AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(3, 3, 0));
	TestFalse(TEXT("Remaining wave prevents completion"),
		AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(3, 2, 0));
	TestFalse(TEXT("Living event enemy prevents completion"),
		AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(3, 3, 1));
	TestFalse(TEXT("Infinite waves never auto-complete"),
		AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(0, 12, 0));
	TestTrue(TEXT("A single empty configured wave completes safely"),
		AGP_RedRiftRegionEventActor::ShouldCompleteAfterWaveSequence(1, 1, 0));

	return true;
}

#endif
