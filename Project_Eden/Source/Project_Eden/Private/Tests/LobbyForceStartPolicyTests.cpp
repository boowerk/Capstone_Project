#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_LobbyGameMode.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLobbyForceStartPolicyTest,
	"ProjectEden.Game.Lobby.ForceStartPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLobbyForceStartPolicyTest::RunTest(const FString& Parameters)
{
	// The debug path must be opt-in and server-authoritative. Remote
	// controllers are accepted only by an explicitly opted-in dedicated server.
	TestFalse(
		TEXT("Explicit launch flag is required"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(false, true, true, false));
	TestFalse(
		TEXT("Authority is required"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, false, true, false));
	TestFalse(
		TEXT("Remote controllers require a dedicated server"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, false, false));

#if UE_BUILD_SHIPPING
	TestFalse(
		TEXT("Shipping builds always disable force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, true, false));
	TestFalse(
		TEXT("Shipping dedicated servers always disable force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, false, true));
#else
	TestTrue(
		TEXT("Explicit local debug host may force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, true, false));
	TestTrue(
		TEXT("Explicit dedicated debug server may force-start"),
		AGP_LobbyGameMode::IsForceStartRequestAllowed(true, true, false, true));
#endif

	return true;
}

#endif
