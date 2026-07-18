#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_LobbyGameMode.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

namespace
{
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

	const AGP_LobbyGameMode* NativeLobbyDefaults = GetDefault<AGP_LobbyGameMode>();
	TestNotNull(TEXT("Native lobby defaults are available"), NativeLobbyDefaults);
	if (NativeLobbyDefaults)
	{
		TestEqual(
			TEXT("Native lobby routes ready parties to the Landscape slice"),
			GameMapNameProperty->GetPropertyValue_InContainer(NativeLobbyDefaults),
			FString(ExpectedLandscapeMapName));
	}

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

	// Blueprint에 직렬화된 이전 경로가 native 기본값을 다시 덮어쓰는 회귀를 함께 막습니다.
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
