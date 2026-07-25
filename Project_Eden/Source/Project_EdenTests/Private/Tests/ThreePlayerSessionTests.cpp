#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_GameMode.h"
#include "Game/GP_LobbyGameMode.h"
#include "Game/GP_ThreePlayerGameSession.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr TCHAR LobbyGameModeBlueprintPath[] =
		TEXT("/Game/GAS_Pattern/Game/BP_LobbyGameMode.BP_LobbyGameMode_C");
	constexpr TCHAR GameplayGameModeBlueprintPath[] =
		TEXT("/Game/GAS_Pattern/Game/BP_ProjectEden_Gamemode.BP_ProjectEden_Gamemode_C");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPThreePlayerSessionConfigurationTest,
	"ProjectEden.Game.Network.ThreePlayerSessionConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPThreePlayerSessionConfigurationTest::RunTest(const FString& Parameters)
{
	const AGP_ThreePlayerGameSession* SessionDefaults = GetDefault<AGP_ThreePlayerGameSession>();
	if (!TestNotNull(TEXT("Three-player GameSession defaults are available"), SessionDefaults))
	{
		return false;
	}

	// Lock the native and config-backed defaults used before URL options are parsed.
	TestEqual(
		TEXT("Session admits exactly three players"),
		SessionDefaults->MaxPlayers,
		AGP_ThreePlayerGameSession::PartyPlayerCapacity);
	TestEqual(
		TEXT("Session admits no network spectators"),
		SessionDefaults->MaxSpectators,
		AGP_ThreePlayerGameSession::PartySpectatorCapacity);
	TestEqual(
		TEXT("Session permits only one player per connection"),
		SessionDefaults->MaxSplitscreensPerConnection,
		AGP_ThreePlayerGameSession::PartySplitscreenCapacity);

	const AGP_LobbyGameMode* NativeLobbyDefaults = GetDefault<AGP_LobbyGameMode>();
	const AGP_GameMode* NativeGameplayDefaults = GetDefault<AGP_GameMode>();
	TestTrue(
		TEXT("Native lobby uses the three-player session"),
		NativeLobbyDefaults
			&& NativeLobbyDefaults->GameSessionClass == AGP_ThreePlayerGameSession::StaticClass());
	TestTrue(
		TEXT("Native gameplay uses the three-player session"),
		NativeGameplayDefaults
			&& NativeGameplayDefaults->GameSessionClass == AGP_ThreePlayerGameSession::StaticClass());

	UClass* LobbyBlueprintClass =
		LoadClass<AGP_LobbyGameMode>(nullptr, LobbyGameModeBlueprintPath);
	const AGP_LobbyGameMode* LobbyBlueprintDefaults = LobbyBlueprintClass
		? Cast<AGP_LobbyGameMode>(LobbyBlueprintClass->GetDefaultObject())
		: nullptr;
	TestTrue(
		TEXT("Production lobby Blueprint keeps the three-player session"),
		LobbyBlueprintDefaults
			&& LobbyBlueprintDefaults->GameSessionClass == AGP_ThreePlayerGameSession::StaticClass());

	UClass* GameplayBlueprintClass =
		LoadClass<AGP_GameMode>(nullptr, GameplayGameModeBlueprintPath);
	const AGP_GameMode* GameplayBlueprintDefaults = GameplayBlueprintClass
		? Cast<AGP_GameMode>(GameplayBlueprintClass->GetDefaultObject())
		: nullptr;
	TestTrue(
		TEXT("Production gameplay Blueprint keeps the three-player session"),
		GameplayBlueprintDefaults
			&& GameplayBlueprintDefaults->GameSessionClass == AGP_ThreePlayerGameSession::StaticClass());

	return true;
}

#endif
