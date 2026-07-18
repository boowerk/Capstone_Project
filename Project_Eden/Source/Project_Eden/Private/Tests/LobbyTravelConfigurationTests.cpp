#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_LobbyGameMode.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr int32 ExpectedPartySize = 3;
	constexpr TCHAR ExpectedLandscapeMapName[] = TEXT("MainMap/L_LandscapeMap");
	constexpr TCHAR LandscapeMapPackagePath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
	constexpr TCHAR LobbyBlueprintClassPath[] =
		TEXT("/Game/GAS_Pattern/Game/BP_LobbyGameMode.BP_LobbyGameMode_C");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLobbyLandscapeTravelConfigurationTest,
	"ProjectEden.Game.Lobby.LandscapeTravelConfiguration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLobbyLandscapeTravelConfigurationTest::RunTest(const FString& Parameters)
{
	const FStrProperty* GameMapNameProperty =
		FindFProperty<FStrProperty>(AGP_LobbyGameMode::StaticClass(), TEXT("GameMapName"));
	if (!TestNotNull(TEXT("Lobby GameMode exposes the travel destination"), GameMapNameProperty))
	{
		return false;
	}

	const FIntProperty* ExpectedPlayerCountProperty =
		FindFProperty<FIntProperty>(AGP_LobbyGameMode::StaticClass(), TEXT("ExpectedPlayerCount"));
	if (!TestNotNull(TEXT("Lobby GameMode exposes the ready-party size"), ExpectedPlayerCountProperty))
	{
		return false;
	}

	const AGP_LobbyGameMode* NativeLobbyDefaults = GetDefault<AGP_LobbyGameMode>();
	TestNotNull(TEXT("Native lobby defaults are available"), NativeLobbyDefaults);
	if (NativeLobbyDefaults)
	{
		// Keep the native ready gate aligned with the three-player product contract.
		TestEqual(
			TEXT("Native lobby requires exactly three connected players"),
			ExpectedPlayerCountProperty->GetPropertyValue_InContainer(NativeLobbyDefaults),
			ExpectedPartySize);
		TestEqual(
			TEXT("Native lobby routes ready parties to the Landscape slice"),
			GameMapNameProperty->GetPropertyValue_InContainer(NativeLobbyDefaults),
			FString(ExpectedLandscapeMapName));
	}

	// Keep the ready gate fail-closed on both sides of the exact party size.
	TestFalse(
		TEXT("Two connected players cannot start the run"),
		AGP_LobbyGameMode::HasExactPartySize(2, ExpectedPartySize));
	TestTrue(
		TEXT("Exactly three connected players satisfy the party-size gate"),
		AGP_LobbyGameMode::HasExactPartySize(3, ExpectedPartySize));
	TestFalse(
		TEXT("Four connected players cannot bypass the party-size gate"),
		AGP_LobbyGameMode::HasExactPartySize(4, ExpectedPartySize));

	UClass* LobbyBlueprintClass = LoadClass<AGP_LobbyGameMode>(nullptr, LobbyBlueprintClassPath);
	if (!TestNotNull(TEXT("Production lobby GameMode Blueprint loads"), LobbyBlueprintClass))
	{
		return false;
	}

	const AGP_LobbyGameMode* LobbyBlueprintDefaults =
		Cast<AGP_LobbyGameMode>(LobbyBlueprintClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Production lobby Blueprint defaults are available"), LobbyBlueprintDefaults))
	{
		return false;
	}

	// Guard against production Blueprint defaults silently overriding either
	// the three-player ready gate or the reviewed travel destination.
	TestEqual(
		TEXT("Production lobby Blueprint requires exactly three connected players"),
		ExpectedPlayerCountProperty->GetPropertyValue_InContainer(LobbyBlueprintDefaults),
		ExpectedPartySize);
	TestEqual(
		TEXT("Production lobby Blueprint routes ready parties to the Landscape slice"),
		GameMapNameProperty->GetPropertyValue_InContainer(LobbyBlueprintDefaults),
		FString(ExpectedLandscapeMapName));

	UPackage* LandscapeMapPackage = LoadPackage(nullptr, LandscapeMapPackagePath, LOAD_None);
	TestNotNull(TEXT("Configured Landscape travel destination package loads"), LandscapeMapPackage);

	TestTrue(
		TEXT("Lobby keeps seamless travel enabled for connected clients"),
		NativeLobbyDefaults && NativeLobbyDefaults->bUseSeamlessTravel);

	return true;
}

#endif
