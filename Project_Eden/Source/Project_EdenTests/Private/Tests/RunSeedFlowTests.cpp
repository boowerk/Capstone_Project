#if WITH_DEV_AUTOMATION_TESTS

#include "Game/GP_GameState.h"
#include "Game/GP_RunSeed.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRunSeedFlowTest,
	"ProjectEden.Game.RunSeed.Flow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRunSeedFlowTest::RunTest(const FString& Parameters)
{
	const TArray<int32> ValidSeeds = { 0, 1, MAX_int32 };
	for (const int32 ExpectedSeed : ValidSeeds)
	{
		const FString URL = GPRunSeed::BuildTravelURL(TEXT("MainMap/L_LandscapeMap"), ExpectedSeed);
		const int32 OptionsStart = URL.Find(TEXT("?"));
		TestTrue(TEXT("Travel URL contains options"), OptionsStart != INDEX_NONE);

		int32 ParsedSeed = INDEX_NONE;
		TestTrue(TEXT("Valid RunSeed option parses"), GPRunSeed::TryParse(URL.Mid(OptionsStart), ParsedSeed));
		TestEqual(TEXT("RunSeed survives travel URL round-trip"), ParsedSeed, ExpectedSeed);
	}

	const TArray<FString> InvalidOptions = {
		TEXT(""),
		TEXT("?listen"),
		TEXT("?RunSeed="),
		TEXT("?RunSeed=-1"),
		TEXT("?RunSeed=+1"),
		TEXT("?RunSeed=1.0"),
		TEXT("?RunSeed=3e10"),
		TEXT("?RunSeed= 1"),
		TEXT("?RunSeed=2147483648")
	};

	for (const FString& Options : InvalidOptions)
	{
		int32 ParsedSeed = INDEX_NONE;
		TestFalse(*FString::Printf(TEXT("Invalid options are rejected: '%s'"), *Options),
			GPRunSeed::TryParse(Options, ParsedSeed));
	}

	int32 CaseInsensitiveSeed = INDEX_NONE;
	TestTrue(TEXT("Unreal URL option keys are case-insensitive"),
		GPRunSeed::TryParse(TEXT("?runseed=1"), CaseInsensitiveSeed));
	TestEqual(TEXT("Lower-case RunSeed option preserves its value"), CaseInsensitiveSeed, 1);

	for (int32 GenerateIndex = 0; GenerateIndex < 64; ++GenerateIndex)
	{
		const int32 GeneratedSeed = GPRunSeed::Generate();
		TestTrue(TEXT("Generated RunSeed is non-negative"), GeneratedSeed >= 0);
	}

	const FProperty* RunSeedProperty = FindFProperty<FProperty>(AGP_GameState::StaticClass(), TEXT("RunSeed"));
	TestNotNull(TEXT("GameState exposes reflected RunSeed storage"), RunSeedProperty);
	if (RunSeedProperty)
	{
		TestTrue(TEXT("GameState RunSeed is replicated"), RunSeedProperty->HasAnyPropertyFlags(CPF_Net));
	}

	TestFalse(TEXT("GameState defaults to an unset RunSeed"), GetDefault<AGP_GameState>()->HasRunSeed());
	return true;
}

#endif
