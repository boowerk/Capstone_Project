#if WITH_DEV_AUTOMATION_TESTS

#include "Game/GP_RunProgressionPolicy.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPartyDefeatPolicyTest,
	"ProjectEden.RunOutcome.PartyDefeatPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPartyDefeatPolicyTest::RunTest(const FString& Parameters)
{
	TestFalse(
		TEXT("An empty dedicated server is not a defeated party"),
		GPRunProgressionPolicy::IsPartyDefeated(0, 0));
	TestFalse(
		TEXT("One eliminated player does not end a three-player run"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 1));
	TestFalse(
		TEXT("One survivor keeps the run alive"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 2));
	TestTrue(
		TEXT("All three eliminated players end the run"),
		GPRunProgressionPolicy::IsPartyDefeated(3, 3));
	TestTrue(
		TEXT("Solo elimination ends a solo run"),
		GPRunProgressionPolicy::IsPartyDefeated(1, 1));
	return true;
}

#endif
