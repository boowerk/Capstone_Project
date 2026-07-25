#if WITH_DEV_AUTOMATION_TESTS

#include "AbilitySystem/Abilities/Enemy/GP_EnemyDeathAbility.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FEnemyDeathLifecycleTest,
	"ProjectEden.Combat.EnemyDeath.Lifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEnemyDeathLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestTrue(TEXT("Created transient enemy death test world"), TestWorld != nullptr))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_EnemyCharacter* Enemy = TestWorld->SpawnActor<AGP_EnemyCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned enemy under test"), Enemy))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	UAbilitySystemComponent* ASC = Enemy->GetAbilitySystemComponent();
	if (!TestNotNull(TEXT("Enemy owns an ASC"), ASC))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	// A minimal transient world does not run BeginPlay, so grant the native death ability explicitly.
	ASC->InitAbilityActorInfo(Enemy, Enemy);
	ASC->GiveAbility(FGameplayAbilitySpec(UGP_EnemyDeathAbility::StaticClass()));

	TestFalse(TEXT("Enemy begins alive"), Enemy->IsDead());
	Enemy->RequestDeath(nullptr);

	TestTrue(TEXT("Death ability enters the terminal state"), Enemy->IsDead());
	TestTrue(TEXT("Death state remains queryable through GAS"), ASC->HasMatchingGameplayTag(GPTags::Ability::Enemy::Death));
	TestFalse(TEXT("Dead enemy collision is disabled"), Enemy->GetActorEnableCollision());
	TestEqual(TEXT("Dead enemy movement is disabled"), Enemy->GetCharacterMovement()->MovementMode, MOVE_None);
	TestTrue(TEXT("Dead enemy is scheduled for despawn"), Enemy->GetLifeSpan() > 0.0f);

	// A repeated request must remain idempotent and preserve the same terminal state.
	Enemy->RequestDeath(nullptr);
	TestTrue(TEXT("Repeated death requests stay dead"), Enemy->IsDead());

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
