#if WITH_DEV_AUTOMATION_TESTS

#include "AI/Controllers/EnemyAIController.h"
#include "AI/Tasks/BTT_RunEnemyPositioningEQSQuery.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Engine/World.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalSeraphPatrolRecoveryTest,
	"ProjectEden.AI.Boss.CrystalSeraph.PatrolRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalSeraphPatrolRecoveryTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient Crystal Seraph patrol test world"), TestWorld))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AGP_CrystalSeraphBossCharacter* Boss = TestWorld->SpawnActor<AGP_CrystalSeraphBossCharacter>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	AActor* DistantTarget = TestWorld->SpawnActor<AActor>(
		FVector(10000.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned Crystal Seraph"), Boss) || !TestNotNull(TEXT("Spawned distant teleport target"), DistantTarget))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	Boss->GetCrystalSeraphStateComponent()->InitializeCrystalSeraphState(Boss);
	const FVector AnchorLocation = Boss->GetBehaviorAnchorLocation();
	TestTrue(TEXT("Tactical teleport succeeds"), Boss->RequestTeleportToPreferredCombatPosition(DistantTarget));
	const float DistanceFromAnchor = FVector::Dist2D(Boss->GetActorLocation(), AnchorLocation);
	TestTrue(
		TEXT("Tactical teleport remains inside the leash re-entry margin"),
		DistanceFromAnchor <= Boss->GetReturnHomeDistance() - Boss->GetReturnHomeAcceptanceRadius() + KINDA_SMALL_NUMBER);

	UBTT_RunEnemyPositioningEQSQuery* PatrolTask = NewObject<UBTT_RunEnemyPositioningEQSQuery>();
	PatrolTask->QueryTemplate = LoadObject<UEnvQuery>(nullptr, TEXT("/Game/Characters/EnemyCharacter/EQS/EQS_PatrolLocation.EQS_PatrolLocation"));
	TestTrue(TEXT("Patrol query is recognized"), PatrolTask->IsPatrolQuery());
	TestTrue(TEXT("Patrol yields while ReturnHome owns movement"), PatrolTask->ShouldYieldToReturnHome(true));
	TestFalse(TEXT("Normal patrol remains available outside ReturnHome"), PatrolTask->ShouldYieldToReturnHome(false));

	Boss->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
	Boss->SetActorLocation(FVector(0.0f, 0.0f, 650.0f));
	const FAIMoveRequest GroundPatrolRequest(FVector(500.0f, 300.0f, 0.0f));
	TestTrue(TEXT("Flying vector movement bypasses ground pathfinding"), AEnemyAIController::ShouldUseDirectFlyingMove(Boss, GroundPatrolRequest));
	TestTrue(
		TEXT("Direct flying movement preserves altitude"),
		FMath::IsNearlyEqual(AEnemyAIController::ResolveDirectFlyingGoal(Boss, GroundPatrolRequest).Z, 650.0));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
