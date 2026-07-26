#if WITH_DEV_AUTOMATION_TESTS

#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Utils/GP_BlueprintLibrary.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCombatFriendlyFireFilterTest,
	"ProjectEden.Combat.FriendlyFire.EnemyTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCombatFriendlyFireFilterTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestTrue(TEXT("Created transient combat filter test world"), TestWorld != nullptr))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_EnemyCharacter* EnemyInstigator = TestWorld->SpawnActor<AGP_EnemyCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
	AGP_EnemyCharacter* EnemyTarget = TestWorld->SpawnActor<AGP_EnemyCharacter>(FVector(100.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	AGP_PlayerCharacter* PlayerInstigator = TestWorld->SpawnActor<AGP_PlayerCharacter>(FVector(200.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);
	AGP_PlayerCharacter* PlayerTarget = TestWorld->SpawnActor<AGP_PlayerCharacter>(FVector(300.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParameters);

	// Enemy friendly fire must stay blocked for boss sweeps, summoned adds, and enemy projectiles.
	TestFalse(TEXT("Enemy cannot apply combat effects to another enemy"), UGP_BlueprintLibrary::CanApplyCombatEffect(EnemyInstigator, EnemyTarget));
	TestTrue(TEXT("Enemy can still apply combat effects to the player"), UGP_BlueprintLibrary::CanApplyCombatEffect(EnemyInstigator, PlayerTarget));
	TestTrue(TEXT("Player can still apply combat effects to enemies"), UGP_BlueprintLibrary::CanApplyCombatEffect(PlayerInstigator, EnemyTarget));
	TestFalse(TEXT("Player cannot apply combat effects to a teammate"), UGP_BlueprintLibrary::CanApplyCombatEffect(PlayerInstigator, PlayerTarget));
	TestFalse(TEXT("Self hits remain blocked"), UGP_BlueprintLibrary::CanApplyCombatEffect(EnemyInstigator, EnemyInstigator));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
