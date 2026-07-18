#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_LobbyGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLobbyForceStartPolicyTest,
	"ProjectEden.Game.Lobby.ForceStartPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLobbyForceStartPolicyTest::RunTest(const FString& Parameters)
{
	// The debug path must be opt-in and must never be delegated to a remote
	// client, even though its RPC executes on an authoritative server object.
	TestFalse(
		TEXT("Explicit launch flag is required"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(false, true, true));
	TestFalse(
		TEXT("Authority is required"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, false, true));
	TestFalse(
		TEXT("Remote controllers cannot force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, false));

#if UE_BUILD_SHIPPING
	TestFalse(
		TEXT("Shipping builds always disable force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, true));
#else
	TestTrue(
		TEXT("Explicit local debug host may force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, true));
#endif

	return true;
}

#endif
