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
#include "UObject/UnrealType.h"

namespace
{
	UWorld* CreatePeripheralTestWorld(const FName WorldName, float FloorHalfExtent)
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, WorldName);
		if (!World)
		{
			return nullptr;
		}
		if (!World->GetPhysicsScene())
		{
			World->CreatePhysicsScene(nullptr);
		}

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Floor = World->SpawnActor<AActor>(FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
		UBoxComponent* FloorCollision = Floor ? NewObject<UBoxComponent>(Floor) : nullptr;
		if (!Floor || !FloorCollision)
		{
			World->DestroyWorld(false);
			return nullptr;
		}

		// A real WorldStatic surface exercises the same ground and capsule checks used by the production landscape.
		Floor->SetRootComponent(FloorCollision);
		FloorCollision->InitBoxExtent(FVector(FloorHalfExtent, FloorHalfExtent, 10.0f));
		FloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		FloorCollision->SetCollisionObjectType(ECC_WorldStatic);
		FloorCollision->SetCollisionResponseToAllChannels(ECR_Block);
		FloorCollision->RegisterComponent();
		return World;
	}

	bool SpawnProductionLikeRegionSeeds(UWorld& World, float CoordinateScale)
	{
		UClass* RegionSeedClass = LoadClass<AActor>(
			nullptr,
			TEXT("/Game/RegionSystem/Blueprints/BP_RegionSeed.BP_RegionSeed_C"));
		if (!RegionSeedClass)
		{
			return false;
		}

		const FVector2D ProductionCoordinates[] =
		{
			FVector2D(-20292.8f, 39230.7f), FVector2D(-9946.5f, -10559.3f),
			FVector2D(-42454.8f, 35745.4f), FVector2D(8639.3f, -44257.4f),
			FVector2D(-29892.0f, -25567.3f), FVector2D(38780.3f, 45738.5f),
			FVector2D(44160.1f, -42024.0f), FVector2D(775.1f, 10604.8f),
			FVector2D(34343.3f, 28560.1f), FVector2D(-27868.8f, 3175.4f),
			FVector2D(40748.3f, -9297.6f), FVector2D(-6754.4f, 27894.7f),
			FVector2D(16562.0f, 43270.2f), FVector2D(19873.5f, -10967.4f),
			FVector2D(4634.3f, -20929.1f)
		};

		for (int32 RegionId = 0; RegionId < UE_ARRAY_COUNT(ProductionCoordinates); ++RegionId)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.Name = *FString::Printf(TEXT("RegionSeed_%d"), RegionId);
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			const FVector2D XY = ProductionCoordinates[RegionId] * CoordinateScale;
			AActor* SeedActor = World.SpawnActor<AActor>(
				RegionSeedClass,
				FVector(XY.X, XY.Y, 500.0f),
				FRotator::ZeroRotator,
				SpawnParameters);
			if (!SeedActor)
			{
				return false;
			}

			// Match the reflected SeedIndex contract used by UGP_RegionSpatialSubsystem.
			FIntProperty* SeedIndexProperty = FindFProperty<FIntProperty>(SeedActor->GetClass(), TEXT("SeedIndex"));
			if (!SeedIndexProperty)
			{
				return false;
			}
			SeedIndexProperty->SetPropertyValue_InContainer(SeedActor, RegionId);
		}
		return true;
	}

	AGP_GameMode* SpawnConfiguredGameMode(UWorld& World, const TCHAR* Options)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AGP_GameMode* GameMode = World.SpawnActor<AGP_GameMode>(
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParameters);
		if (!GameMode)
		{
			return nullptr;
		}

		GameMode->DefaultPawnClass = ACharacter::StaticClass();
		FString ErrorMessage;
		// The transient package is named Untitled, while the world object preserves the production-map token.
		GameMode->InitGame(World.GetName(), Options, ErrorMessage);
		return ErrorMessage.IsEmpty() ? GameMode : nullptr;
	}

	TArray<AActor*> AssignThreeStarts(UWorld& World, AGP_GameMode& GameMode)
	{
		TArray<AActor*> AssignedStarts;
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		for (int32 PlayerIndex = 0; PlayerIndex < 3; ++PlayerIndex)
		{
			APlayerController* Controller = World.SpawnActor<APlayerController>(
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
			AssignedStarts.Add(Controller ? GameMode.ChoosePlayerStart(Controller) : nullptr);
		}
		return AssignedStarts;
	}

	void CountPlayerStarts(UWorld& World, int32& OutTotal, int32& OutRuntime, int32& OutPeripheral)
	{
		OutTotal = 0;
		OutRuntime = 0;
		OutPeripheral = 0;
		for (TActorIterator<APlayerStart> It(&World); It; ++It)
		{
			++OutTotal;
			OutRuntime += It->Tags.Contains(TEXT("GP.RuntimePartyStart")) ? 1 : 0;
			OutPeripheral += It->Tags.Contains(TEXT("GP.PeripheralPartyStart")) ? 1 : 0;
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPeripheralPartyStartTest,
	"ProjectEden.Game.DemoFlow.PeripheralPartyStarts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPeripheralPartyStartTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreatePeripheralTestWorld(TEXT("L_LandscapeMap_PeripheralTest"), 20000.0f);
	if (!TestNotNull(TEXT("Created peripheral spawn world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* AuthoredStart = World->SpawnActor<APlayerStart>(
		FVector(0.0f, 0.0f, 108.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_GameMode* GameMode = SpawnConfiguredGameMode(*World, TEXT("?DemoSeed=-4242"));
	if (!TestNotNull(TEXT("Spawned authored fallback anchor"), AuthoredStart)
		|| !TestTrue(TEXT("Spawned all production-like seeds"), SpawnProductionLikeRegionSeeds(*World, 0.2f))
		|| !TestNotNull(TEXT("Spawned configured GameMode"), GameMode))
	{
		World->DestroyWorld(false);
		return false;
	}

	const TArray<AActor*> AssignedStarts = AssignThreeStarts(*World, *GameMode);
	TestEqual(TEXT("Explicit negative demo seeds remain valid"), GameMode->GetDemoRunSeed(), -4242);
	TestTrue(TEXT("A production-like map creates a valid demo route"), GameMode->HasValidDemoRunRoute());
	TestEqual(TEXT("Three players receive three peripheral starts"), AssignedStarts.Num(), 3);

	TSet<AActor*> UniqueStarts;
	for (AActor* AssignedStart : AssignedStarts)
	{
		TestNotNull(TEXT("Peripheral assignment is valid"), AssignedStart);
		if (!AssignedStart)
		{
			continue;
		}
		UniqueStarts.Add(AssignedStart);
		TestTrue(TEXT("Assigned start is marked as peripheral"), AssignedStart->Tags.Contains(TEXT("GP.PeripheralPartyStart")));
		const FVector TowardCenter =
			(GameMode->GetDemoRunRoute().CenterLocation - AssignedStart->GetActorLocation()).GetSafeNormal2D();
		TestTrue(
			TEXT("Every party start faces inward"),
			FVector::DotProduct(AssignedStart->GetActorForwardVector(), TowardCenter) >= 0.98f);
	}
	TestEqual(TEXT("All three peripheral start actors are unique"), UniqueStarts.Num(), 3);
	for (int32 FirstIndex = 0; FirstIndex < AssignedStarts.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < AssignedStarts.Num(); ++SecondIndex)
		{
			if (AssignedStarts[FirstIndex] && AssignedStarts[SecondIndex])
			{
				TestTrue(
					TEXT("Peripheral cluster slots remain at least 150 cm apart"),
					FVector::Dist2D(
						AssignedStarts[FirstIndex]->GetActorLocation(),
						AssignedStarts[SecondIndex]->GetActorLocation()) >= 150.0f);
			}
		}
	}

	int32 TotalStarts = 0;
	int32 RuntimeStarts = 0;
	int32 PeripheralStarts = 0;
	CountPlayerStarts(*World, TotalStarts, RuntimeStarts, PeripheralStarts);
	TestEqual(TEXT("Authored anchor remains in the map beside three runtime starts"), TotalStarts, 4);
	TestEqual(TEXT("All three selected slots are runtime starts"), RuntimeStarts, 3);
	TestEqual(TEXT("All runtime slots belong to the peripheral cluster"), PeripheralStarts, 3);

	World->DestroyWorld(false);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPPeripheralPartyStartFallbackTest,
	"ProjectEden.Game.DemoFlow.PeripheralPartyStartFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPPeripheralPartyStartFallbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = CreatePeripheralTestWorld(TEXT("L_LandscapeMap_PeripheralFallbackTest"), 1500.0f);
	if (!TestNotNull(TEXT("Created fallback spawn world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* AuthoredStart = World->SpawnActor<APlayerStart>(
		FVector(0.0f, 0.0f, 108.0f),
		FRotator::ZeroRotator,
		SpawnParameters);
	AGP_GameMode* GameMode = SpawnConfiguredGameMode(*World, TEXT("?DemoSeed=77"));
	if (!TestNotNull(TEXT("Spawned fallback anchor"), AuthoredStart)
		|| !TestTrue(TEXT("Spawned unsafe outer seeds"), SpawnProductionLikeRegionSeeds(*World, 0.2f))
		|| !TestNotNull(TEXT("Spawned fallback GameMode"), GameMode))
	{
		World->DestroyWorld(false);
		return false;
	}

	const TArray<AActor*> AssignedStarts = AssignThreeStarts(*World, *GameMode);
	TestFalse(TEXT("Unsafe peripheral ground does not publish a route"), GameMode->HasValidDemoRunRoute());
	for (AActor* AssignedStart : AssignedStarts)
	{
		TestNotNull(TEXT("Authored fallback still assigns every player"), AssignedStart);
	}

	int32 TotalStarts = 0;
	int32 RuntimeStarts = 0;
	int32 PeripheralStarts = 0;
	CountPlayerStarts(*World, TotalStarts, RuntimeStarts, PeripheralStarts);
	TestEqual(TEXT("Fallback preserves the established three-start contract"), TotalStarts, 3);
	TestEqual(TEXT("Fallback creates only two runtime companions"), RuntimeStarts, 2);
	TestEqual(TEXT("Failed candidates leave no peripheral start actors"), PeripheralStarts, 0);

	World->DestroyWorld(false);
	return !HasAnyErrors();
}

#endif
