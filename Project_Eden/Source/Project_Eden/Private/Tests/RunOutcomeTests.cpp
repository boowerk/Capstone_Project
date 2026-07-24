#if WITH_DEV_AUTOMATION_TESTS

#include "Game/GP_GameMode.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPartyDefeatPolicyTest,
	"ProjectEden.RunOutcome.PartyDefeatPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPartyDefeatPolicyTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("An empty dedicated server is not a defeated party"),
		AGP_GameMode::IsPartyDefeated(0, 0));
	TestFalse(
		TEXT("One eliminated player does not end a three-player run"),
		AGP_GameMode::IsPartyDefeated(3, 1));
	TestFalse(
		TEXT("One survivor keeps the run alive"),
		AGP_GameMode::IsPartyDefeated(3, 2));
	TestTrue(
		TEXT("All three eliminated players end the run"),
		AGP_GameMode::IsPartyDefeated(3, 3));
	TestTrue(
		TEXT("Solo elimination ends a solo run"),
		AGP_GameMode::IsPartyDefeated(1, 1));
	return true;
}

#endif
