#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameMode.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunPortal.h"
#include "Game/GP_RunProgressionPolicy.h"
#include "Game/GP_ZoneRuntimeState.h"
#include "Game/WorldLayout/GP_VillageLayoutDirector.h"
#include "Player/GP_PlayerController.h"
#include "UI/GP_MiddleTravelMapWidget.h"

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
	TestNotNull(
		TEXT("GameMode exposes the authored village portal anchor tag"),
		AGP_GameMode::StaticClass()->FindPropertyByName(
			TEXT("VillagePortalAnchorTag")));
	TestNotNull(
		TEXT("GameMode exposes the authored Middle arrival anchor tag"),
		AGP_GameMode::StaticClass()->FindPropertyByName(
			TEXT("MiddleArrivalAnchorTag")));
	TestNotNull(
		TEXT("PlayerController exposes a Middle travel map widget class"),
		AGP_PlayerController::StaticClass()->FindPropertyByName(
			TEXT("MiddleTravelMapWidgetClass")));
	TestNotNull(
		TEXT("Middle travel portal mode replicates to clients"),
		AGP_RunPortal::StaticClass()->FindPropertyByName(
			TEXT("bMiddleDestinationSelection")));
	TestNotNull(
		TEXT("Middle travel map has a native Widget Blueprint base"),
		UGP_MiddleTravelMapWidget::StaticClass());

	const FGPZoneRuntimeState RuntimeDefaults;
	TestEqual(TEXT("Zone runtime begins with no enemies"), RuntimeDefaults.AliveEnemies, 0);
	TestEqual(
		TEXT("Zone runtime begins with no pending enemy spawns"),
		RuntimeDefaults.PendingEnemySpawns,
		0);
	TestEqual(
		TEXT("Zone runtime begins with no boss-spawn retries"),
		RuntimeDefaults.BossSpawnRetryCount,
		0);
	TestFalse(
		TEXT("Zone runtime begins without a pending boss-spawn retry"),
		RuntimeDefaults.bBossSpawnRetryPending);
	TestFalse(TEXT("Zone runtime begins inactive"), RuntimeDefaults.bStarted);
	TestFalse(TEXT("Zone runtime begins uncleared"), RuntimeDefaults.bCompleted);

	TestTrue(
		TEXT("A solo run advances after its one assigned Outer zone clears"),
		GPRunProgressionPolicy::IsOuterStageReady(1, 1, 1));
	TestTrue(
		TEXT("A two-player run ignores the third unassigned Outer candidate"),
		GPRunProgressionPolicy::IsOuterStageReady(2, 2, 2));
	TestFalse(
		TEXT("A party waits until every active player's assigned Outer zone clears"),
		GPRunProgressionPolicy::IsOuterStageReady(3, 3, 2));
	TestTrue(
		TEXT("A three-player party advances after its three assigned Outer zones clear"),
		GPRunProgressionPolicy::IsOuterStageReady(3, 3, 3));
	TestFalse(
		TEXT("Missing or duplicate Outer assignments cannot advance the stage"),
		GPRunProgressionPolicy::IsOuterStageReady(3, 2, 2));
	TestFalse(
		TEXT("An empty server cannot advance the Outer stage"),
		GPRunProgressionPolicy::IsOuterStageReady(0, 0, 0));
	return true;
}

#endif
