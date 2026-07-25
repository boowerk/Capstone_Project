#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Game/GP_GameMode.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPThreePlayerRuntimeStartTest,
	"ProjectEden.Game.Network.ThreePlayerRuntimeStarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPThreePlayerRuntimeStartTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created three-player spawn test world"), TestWorld))
	{
		return false;
	}
	// CreateWorld already owns a PersistentLevel and WorldSettings; only add the physics scene when absent.
	if (!TestWorld->GetPhysicsScene())
	{
		TestWorld->CreatePhysicsScene(nullptr);
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AActor* TestFloor = TestWorld->SpawnActor<AActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	UBoxComponent* FloorCollision = TestFloor ? NewObject<UBoxComponent>(TestFloor) : nullptr;
	if (TestFloor && FloorCollision)
	{
		// Supply real WorldStatic geometry so the runtime ground and capsule-fit checks execute in the test.
		TestFloor->SetRootComponent(FloorCollision);
		FloorCollision->InitBoxExtent(FVector(1500.0f, 1500.0f, 10.0f));
		FloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FloorCollision->SetCollisionObjectType(ECC_WorldStatic);
		FloorCollision->SetCollisionResponseToAllChannels(ECR_Block);
		FloorCollision->RegisterComponent();
	}

	AGP_GameMode* GameMode = TestWorld->SpawnActor<AGP_GameMode>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParameters);
	APlayerStart* AuthoredAnchor = TestWorld->SpawnActor<APlayerStart>(
		FVector(0.0f, 0.0f, 108.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!TestNotNull(TEXT("Spawned collision floor"), TestFloor)
		|| !TestNotNull(TEXT("Registered collision floor component"), FloorCollision)
		|| !TestNotNull(TEXT("Spawned gameplay GameMode"), GameMode)
		|| !TestNotNull(TEXT("Spawned authored PlayerStart anchor"), AuthoredAnchor))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	// A native Character CDO supplies capsule-fit checks without loading production Blueprint content.
	GameMode->DefaultPawnClass = ACharacter::StaticClass();

	TArray<APlayerController*> Controllers;
	TArray<AActor*> AssignedStarts;
	for (int32 PlayerIndex = 0; PlayerIndex < 3; ++PlayerIndex)
	{
		APlayerController* Controller = TestWorld->SpawnActor<APlayerController>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
		Controllers.Add(Controller);
		AssignedStarts.Add(GameMode->ChoosePlayerStart(Controller));
	}

	TestEqual(TEXT("Three controllers receive three start assignments"), AssignedStarts.Num(), 3);
	for (int32 PlayerIndex = 0; PlayerIndex < AssignedStarts.Num(); ++PlayerIndex)
	{
		TestNotNull(*FString::Printf(TEXT("Player %d receives a valid start"), PlayerIndex), AssignedStarts[PlayerIndex]);
	}

	TSet<AActor*> UniqueStarts;
	for (AActor* AssignedStart : AssignedStarts)
	{
		UniqueStarts.Add(AssignedStart);
	}
	TestEqual(TEXT("Every player receives a distinct start actor"), UniqueStarts.Num(), 3);
	for (int32 FirstIndex = 0; FirstIndex < AssignedStarts.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < AssignedStarts.Num(); ++SecondIndex)
		{
			if (AssignedStarts[FirstIndex] && AssignedStarts[SecondIndex])
			{
				const float Separation = FVector::Dist(
					AssignedStarts[FirstIndex]->GetActorLocation(),
					AssignedStarts[SecondIndex]->GetActorLocation());
				TestTrue(TEXT("Runtime party starts remain at least 150 cm apart"), Separation >= 150.0f);
			}
		}
	}

	TestTrue(
		TEXT("Repeated start lookup preserves the controller's stable slot"),
		Controllers.IsValidIndex(1)
			&& AssignedStarts.IsValidIndex(1)
			&& GameMode->ChoosePlayerStart(Controllers[1]) == AssignedStarts[1]);

	TSet<APawn*> SpawnedPawns;
	for (APlayerController* Controller : Controllers)
	{
		if (IsValid(Controller))
		{
			GameMode->RestartPlayer(Controller);
			APawn* Pawn = Controller->GetPawn();
			TestNotNull(TEXT("RestartPlayer creates a possessed Pawn"), Pawn);
			TestTrue(TEXT("Spawned Pawn is owned by its assigned controller"), Pawn && Pawn->GetController() == Controller);
			SpawnedPawns.Add(Pawn);
		}
	}
	TestEqual(TEXT("Three controllers possess three distinct Pawns"), SpawnedPawns.Num(), 3);

	int32 PlayerStartCount = 0;
	int32 RuntimePlayerStartCount = 0;
	for (TActorIterator<APlayerStart> It(TestWorld); It; ++It)
	{
		++PlayerStartCount;
		if (It->Tags.Contains(TEXT("GP.RuntimePartyStart")))
		{
			++RuntimePlayerStartCount;
		}
	}
	TestEqual(TEXT("One authored anchor expands to three total starts"), PlayerStartCount, 3);
	TestEqual(TEXT("Exactly two transient runtime starts are added"), RuntimePlayerStartCount, 2);

	TestWorld->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
