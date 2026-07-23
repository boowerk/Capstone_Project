#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameMode.h"
#include "Game/GP_GameState.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPZoneProgressionContractTest,
	"ProjectEden.Game.ZoneProgression.Contracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPZoneProgressionContractTest::RunTest(const FString& Parameters)
{
	const AGP_EnemySpawnVolume* ZoneDefaults = GetDefault<AGP_EnemySpawnVolume>();
	TestEqual(
		TEXT("Existing authored zones retain legacy linear behavior"),
		ZoneDefaults->GetZoneStage(),
		EGPZoneStage::Legacy);

	TestNotNull(
		TEXT("Zones expose a stable runtime zone id"),
		AGP_EnemySpawnVolume::StaticClass()->FindPropertyByName(TEXT("ZoneId")));
	TestNotNull(
		TEXT("Zones expose a progression stage"),
		AGP_EnemySpawnVolume::StaticClass()->FindPropertyByName(TEXT("ZoneStage")));
	TestNotNull(
		TEXT("Zone entry reports the entering player"),
		AGP_EnemySpawnVolume::StaticClass()->FindPropertyByName(
			TEXT("OnPlayerEnteredZoneDetailed")));
	TestNotNull(
		TEXT("Zone exit reports the leaving player"),
		AGP_EnemySpawnVolume::StaticClass()->FindPropertyByName(
			TEXT("OnPlayerExitedZoneDetailed")));

	TestNotNull(
		TEXT("Middle qualification is replicated through GameState"),
		AGP_GameState::StaticClass()->FindPropertyByName(TEXT("MiddleQualifiedPlayers")));
	TestNotNull(
		TEXT("Village layout exposes its runtime-ready signal"),
		AGP_VillageLayoutDirector::StaticClass()->FindPropertyByName(
			TEXT("OnRuntimeLayoutReady")));
	TestNotNull(
		TEXT("GameMode exposes zone-only navigation invokers"),
		AGP_GameMode::StaticClass()->FindPropertyByName(
			TEXT("bUseZoneNavigationInvokers")));
	TestNotNull(
		TEXT("GameMode exposes the zone navigation generation radius"),
		AGP_GameMode::StaticClass()->FindPropertyByName(
			TEXT("ZoneNavigationGenerationRadius")));
	TestNotNull(
		TEXT("GameMode exposes the zone navigation removal radius"),
		AGP_GameMode::StaticClass()->FindPropertyByName(
			TEXT("ZoneNavigationRemovalRadius")));

	const FGPZoneRuntimeState RuntimeDefaults;
	TestEqual(TEXT("Zone runtime begins with no enemies"), RuntimeDefaults.AliveEnemies, 0);
	TestFalse(TEXT("Zone runtime begins inactive"), RuntimeDefaults.bStarted);
	TestFalse(TEXT("Zone runtime begins uncleared"), RuntimeDefaults.bCompleted);
	return true;
}

#endif
