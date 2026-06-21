#if WITH_DEV_AUTOMATION_TESTS

#include "Actors/GP_CrystalPrismActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphGroggyLifecycleTest,
	"ProjectEden.Combat.CrystalSeraph.GroggyLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphGroggyLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient Crystal Seraph test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_CrystalSeraphBossCharacter* Boss = TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
		FVector(0.0f, 0.0f, 1000.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_CrystalPrismActor* Prism = TestWorld->SpawnActor<AGP_CrystalPrismActor>(
		FVector(300.0f, 0.0f, 100.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned Crystal Seraph"), Boss) || !TestNotNull(TEXT("Spawned reflection prism"), Prism))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UGP_CrystalSeraphStateComponent* StateComponent = Boss->GetCrystalSeraphStateComponent();
	if (!TestNotNull(TEXT("Crystal Seraph owns its state component"), StateComponent))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	StateComponent->InitializeCrystalSeraphState(Boss);
	Prism->InitializePrism(Boss);
	// Reflection success is counted once per laser pattern and reaches groggy exactly on the third hit.
	TestTrue(TEXT("First reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestEqual(TEXT("First laser breaks one stage"), StateComponent->GetWingCoreBreakCount(), 1);
	TestTrue(TEXT("Second reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestEqual(TEXT("Second laser breaks two stages"), StateComponent->GetWingCoreBreakCount(), 2);
	TestFalse(TEXT("Boss remains airborne before the third hit"), StateComponent->IsGroggy());
	TestTrue(TEXT("Third reflected laser is accepted"), Prism->NotifyLaserHit(nullptr, FVector::ForwardVector));
	TestTrue(TEXT("Third laser enters groggy"), StateComponent->IsGroggy());

	// Exercise the boss-owned presentation and recovery gate without requiring BeginPlay in the transient world.
	Boss->HandleCrystalSeraphGroggyChanged(true);
	TestEqual(TEXT("Groggy starts as a physical fall"), Boss->GetCharacterMovement()->MovementMode, MOVE_Falling);
	TestFalse(TEXT("Recovery is not scheduled before player damage"), Boss->IsGroggyRecoveryScheduled());

	APlayerState* PlayerDamageInstigator = TestWorld->SpawnActor<APlayerState>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (TestNotNull(TEXT("Spawned player damage instigator"), PlayerDamageInstigator))
	{
		Boss->HandlePostDamageTaken(PlayerDamageInstigator, 10.0f, FGameplayTag());
		TestFalse(TEXT("A player hit during the fall does not schedule recovery"), Boss->IsGroggyRecoveryScheduled());

		// Landing changes the temporary fall mode to walking before the required vulnerability hit can count.
		Boss->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		Boss->HandlePostDamageTaken(PlayerDamageInstigator, 10.0f, FGameplayTag());
		TestTrue(TEXT("First post-landing player hit schedules recovery"), Boss->IsGroggyRecoveryScheduled());
		TestTrue(TEXT("Recovery delay owns an active timer"), TestWorld->GetTimerManager().IsTimerActive(Boss->GroggyRecoveryTimerHandle));

		// The timer callback uses the same recovery entry point that restores guarded airborne combat.
		Boss->RequestRecoverFromGroggy();
		TestFalse(TEXT("Recovery clears groggy"), StateComponent->IsGroggy());
		TestFalse(TEXT("Recovery consumes the scheduled gate"), Boss->IsGroggyRecoveryScheduled());
		TestEqual(TEXT("Recovered boss returns to flying movement"), Boss->GetCharacterMovement()->MovementMode, MOVE_Flying);
	}

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
