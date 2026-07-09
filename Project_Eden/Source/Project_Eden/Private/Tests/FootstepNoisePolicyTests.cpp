#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Perception/FootstepNoisePolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFootstepNoisePolicyTest,
	"ProjectEden.AI.Perception.FootstepNoise",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFootstepNoisePolicyTest::RunTest(const FString& Parameters)
{
	FGPFootstepNoiseSettings Settings;
	TestFalse(TEXT("Standing still does not emit a footstep"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 0.0f, true, false, 1.0f, 0.0f));
	TestFalse(TEXT("Airborne movement does not emit a footstep"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 500.0f, false, false, 1.0f, 0.0f));
	TestTrue(TEXT("Movement start emits immediately"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 200.0f, true, false, 1.0f, -1000000.0f));
	TestFalse(TEXT("Walking respects its interval"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 200.0f, true, false, 1.30f, 1.0f));
	TestTrue(TEXT("Walking emits after its interval"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 200.0f, true, false, 1.43f, 1.0f));
	TestTrue(TEXT("Sprinting emits more frequently than walking"),
		FootstepNoisePolicy::ShouldReportNoise(Settings, 600.0f, true, true, 1.30f, 1.0f));

	const float WalkLoudness = FootstepNoisePolicy::ResolveLoudness(Settings, false, false);
	const float SprintLoudness = FootstepNoisePolicy::ResolveLoudness(Settings, true, false);
	const float CrouchedLoudness = FootstepNoisePolicy::ResolveLoudness(Settings, false, true);
	TestTrue(TEXT("Sprint is at least as loud as walking"), SprintLoudness >= WalkLoudness);
	TestTrue(TEXT("Crouching reduces footstep hearing range"), CrouchedLoudness < WalkLoudness);

	return true;
}

#endif
