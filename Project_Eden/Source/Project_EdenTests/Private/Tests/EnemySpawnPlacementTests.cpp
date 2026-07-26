#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Navigation/GP_GroundPlacement.h"
#include "Tests/AutomationCommon.h"

#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPEnemySpawnGroundPolicyTest,
	"ProjectEden.Game.EnemySpawnPlacement.GroundPolicy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPEnemySpawnGroundPolicyTest::RunTest(const FString& Parameters)
{
	const AGP_EnemySpawnVolume* ZoneDefaults = GetDefault<AGP_EnemySpawnVolume>();
	const FVector GroundAnchor = FVector::ZeroVector;

	TestTrue(
		TEXT("Shared ground policy accepts ordinary connected-ground rise"),
		GPGroundPlacement::IsGroundRiseWithinLimit(
			GroundAnchor,
			FVector(0.0f, 0.0f, 250.0f),
			250.0f));
	TestFalse(
		TEXT("Shared ground policy rejects roof-height rise"),
		GPGroundPlacement::IsGroundRiseWithinLimit(
			GroundAnchor,
			FVector(0.0f, 0.0f, 251.0f),
			250.0f));
	TestTrue(
		TEXT("Shared ground policy permits downward Level Instance correction"),
		GPGroundPlacement::IsGroundRiseWithinLimit(
			GroundAnchor,
			FVector(0.0f, 0.0f, -1000.0f),
			250.0f));
	TestFalse(
		TEXT("Shared ground policy rejects non-finite candidates"),
		GPGroundPlacement::IsGroundRiseWithinLimit(
			GroundAnchor,
			FVector(
				std::numeric_limits<FVector::FReal>::quiet_NaN(),
				0.0f,
				0.0f),
			250.0f));

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPFlyingEnemySpawnPlacementTest,
	"ProjectEden.Game.EnemySpawnPlacement.FlyingBeginPlay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPFlyingEnemySpawnPlacementTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Created flying-enemy test world"), WorldWrapper.CreateTestWorld(EWorldType::Game))
		|| !TestNotNull(TEXT("Flying-enemy test world is valid"), WorldWrapper.GetTestWorld())
		|| !TestTrue(TEXT("Started play in flying-enemy test world"), WorldWrapper.BeginPlayInTestWorld()))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	UWorld* TestWorld = WorldWrapper.GetTestWorld();
	const FVector RequestedNavLocation(0.0f, 0.0f, 500.0f);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AGP_CrystalSeraphBossCharacter* Boss =
		TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
			RequestedNavLocation,
			FRotator::ZeroRotator,
			SpawnParameters);
	if (!TestNotNull(TEXT("Spawned flying Crystal Seraph"), Boss))
	{
		WorldWrapper.ForwardErrorMessages(this);
		return false;
	}

	FVector HoveredGroundLocation = Boss->GetActorLocation();
	if (const UCapsuleComponent* Capsule = Boss->GetCapsuleComponent())
	{
		HoveredGroundLocation.Z -= Capsule->GetScaledCapsuleHalfHeight();
	}

	const FVector PlacementGroundLocation = Boss->GetSpawnPlacementGroundLocation();
	const AGP_EnemySpawnVolume* ZoneDefaults = GetDefault<AGP_EnemySpawnVolume>();
	TestTrue(
		TEXT("Crystal Seraph intentionally rises beyond the grounded spawn tolerance"),
		HoveredGroundLocation.Z - PlacementGroundLocation.Z > 50.0f);
	TestTrue(
		TEXT("Pre-BeginPlay collision-adjusted placement remains valid"),
		ZoneDefaults->IsFinalSpawnGroundLocationValid(
			RequestedNavLocation,
			PlacementGroundLocation));
	TestFalse(
		TEXT("Using the post-BeginPlay hover transform would reproduce the rejection"),
		ZoneDefaults->IsFinalSpawnGroundLocationValid(
			RequestedNavLocation,
			HoveredGroundLocation));

	WorldWrapper.ForwardErrorMessages(this);
	return !HasAnyErrors();
}

#endif
