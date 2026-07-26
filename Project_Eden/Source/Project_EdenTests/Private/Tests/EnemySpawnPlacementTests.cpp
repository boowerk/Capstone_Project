#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Game/GP_EnemySpawnVolume.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPEnemySpawnGroundPolicyTest,
	"ProjectEden.Game.EnemySpawnPlacement.GroundPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPEnemySpawnGroundPolicyTest::RunTest(const FString& Parameters)
{
	const AGP_EnemySpawnVolume* ZoneDefaults = GetDefault<AGP_EnemySpawnVolume>();
	const FVector GroundAnchor = FVector::ZeroVector;

	TestTrue(
		TEXT("Normal connected-ground elevation remains valid"),
		ZoneDefaults->IsSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, 200.0f)));
	TestFalse(
		TEXT("Roof-height elevation above the ground anchor is rejected"),
		ZoneDefaults->IsSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, 300.0f)));
	TestTrue(
		TEXT("Downward projection remains valid for vertically offset Level Instances"),
		ZoneDefaults->IsSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, -1000.0f)));
	TestFalse(
		TEXT("Candidates outside the spawn box are rejected"),
		ZoneDefaults->IsSpawnGroundLocationValid(
			GroundAnchor,
			FVector(600.0f, 0.0f, 0.0f)));
	TestTrue(
		TEXT("Small post-spawn floor adjustment remains valid"),
		ZoneDefaults->IsFinalSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, 40.0f)));
	TestFalse(
		TEXT("Stacked or elevated post-spawn placement is rejected"),
		ZoneDefaults->IsFinalSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, 75.0f)));
	TestTrue(
		TEXT("Collision fallback at or below the requested nav height does not lose encounters"),
		ZoneDefaults->IsFinalSpawnGroundLocationValid(
			GroundAnchor,
			FVector(100.0f, 100.0f, -100.0f)));

	return true;
}

#endif
