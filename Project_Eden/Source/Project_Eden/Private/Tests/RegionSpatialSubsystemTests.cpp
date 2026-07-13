#if WITH_DEV_AUTOMATION_TESTS

#include "Game/Regions/GP_RegionSpatialSubsystem.h"

#include "Engine/World.h"
#include "Game/GP_GameState.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPRegionSpatialSubsystemTest,
	"ProjectEden.Game.Region.SpatialSubsystem",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

namespace
{
	AActor* SpawnRegionSeed(
		UWorld& World,
		UClass& RegionSeedClass,
		const FName ActorName,
		const int32 RegionId,
		const uint8 AuthoredState,
		const FVector& Location)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Name = ActorName;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* SeedActor = World.SpawnActor<AActor>(&RegionSeedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!IsValid(SeedActor))
		{
			return nullptr;
		}

		// Set the Blueprint variable through the same reflected contract used by the runtime subsystem.
		if (FIntProperty* SeedIndexProperty = FindFProperty<FIntProperty>(SeedActor->GetClass(), TEXT("SeedIndex")))
		{
			SeedIndexProperty->SetPropertyValue_InContainer(SeedActor, RegionId);
		}
		FProperty* StateProperty = SeedActor->GetClass()->FindPropertyByName(TEXT("State"));
		FNumericProperty* NumericStateProperty = CastField<FNumericProperty>(StateProperty);
		if (FEnumProperty* EnumStateProperty = CastField<FEnumProperty>(StateProperty))
		{
			NumericStateProperty = EnumStateProperty->GetUnderlyingProperty();
		}
		if (StateProperty && NumericStateProperty && NumericStateProperty->IsInteger())
		{
			// Set the actual Blueprint numeric storage; State may be byte, enum-backed byte, or an integer after migration.
			void* StateAddress = StateProperty->ContainerPtrToValuePtr<void>(SeedActor);
			NumericStateProperty->SetIntPropertyValue(StateAddress, static_cast<uint64>(AuthoredState));
		}
		return SeedActor;
	}
}

bool FGPRegionSpatialSubsystemTest::RunTest(const FString& Parameters)
{
	UWorld* TestWorld = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Created transient region test world"), TestWorld))
	{
		return false;
	}

	UClass* RegionSeedClass = LoadClass<AActor>(
		nullptr,
		TEXT("/Game/RegionSystem/Blueprints/BP_RegionSeed.BP_RegionSeed_C"));
	if (!TestNotNull(TEXT("Loaded BP_RegionSeed generated class"), RegionSeedClass))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	AActor* RegionTwo = SpawnRegionSeed(
		*TestWorld,
		*RegionSeedClass,
		TEXT("RegionSeed_2_Primary"),
		2,
		1,
		FVector(0.0f, 0.0f, 5000.0f));
	AActor* RegionSeven = SpawnRegionSeed(
		*TestWorld,
		*RegionSeedClass,
		TEXT("RegionSeed_7_Primary"),
		7,
		5,
		FVector(1000.0f, 0.0f, -5000.0f));
	AActor* DuplicateSeven = SpawnRegionSeed(
		*TestWorld,
		*RegionSeedClass,
		TEXT("RegionSeed_7_Secondary"),
		7,
		6,
		FVector(1600.0f, 0.0f, 0.0f));
	AActor* NegativeSeed = SpawnRegionSeed(
		*TestWorld,
		*RegionSeedClass,
		TEXT("RegionSeed_Invalid"),
		INDEX_NONE,
		7,
		FVector(500.0f, 0.0f, 0.0f));
	TestNotNull(TEXT("Spawned region 2 seed"), RegionTwo);
	TestNotNull(TEXT("Spawned region 7 seed"), RegionSeven);
	TestNotNull(TEXT("Spawned duplicate region seed"), DuplicateSeven);
	TestNotNull(TEXT("Spawned negative region seed"), NegativeSeed);

	UGP_RegionSpatialSubsystem* RegionSubsystem = TestWorld->GetSubsystem<UGP_RegionSpatialSubsystem>();
	if (!TestNotNull(TEXT("Transient world owns a region spatial subsystem"), RegionSubsystem))
	{
		TestWorld->DestroyWorld(false);
		return false;
	}

	RegionSubsystem->RefreshRegionSeeds();
	TestEqual(TEXT("Duplicate and negative ids do not create cache entries"), RegionSubsystem->GetSeedCount(), 2);
	TestEqual(TEXT("RegionCount can index the highest valid RegionId"), RegionSubsystem->GetRegionCount(), 8);
	TestEqual(
		TEXT("Nearest-seed lookup ignores Z and resolves region 7"),
		RegionSubsystem->ResolveRegionIdAtLocation(FVector(900.0f, 0.0f, 100000.0f)),
		7);
	TestEqual(
		TEXT("Exact XY ties resolve to the lower deterministic RegionId"),
		RegionSubsystem->ResolveRegionIdAtLocation(FVector(500.0f, 0.0f, 0.0f)),
		2);

	FVector RegionSevenLocation = FVector::ZeroVector;
	TestTrue(TEXT("Cached seed location is exposed by RegionId"), RegionSubsystem->GetSeedLocation(7, RegionSevenLocation));
	TestTrue(
		TEXT("Deterministic duplicate handling keeps the lexically first actor"),
		RegionSevenLocation.Equals(FVector(1000.0f, 0.0f, -5000.0f), KINDA_SMALL_NUMBER));
	TestFalse(TEXT("Missing RegionId has no seed location"), RegionSubsystem->GetSeedLocation(4, RegionSevenLocation));

	uint8 RegionSevenState = 0;
	TestTrue(TEXT("Authored seed state is available by RegionId"), RegionSubsystem->GetAuthoredRegionState(7, RegionSevenState));
	TestEqual(TEXT("Deterministic duplicate handling also preserves the primary seed state"), RegionSevenState, static_cast<uint8>(5));
	TestFalse(TEXT("Missing RegionId has no authored state"), RegionSubsystem->GetAuthoredRegionState(4, RegionSevenState));

	TArray<uint8> InitialRegionStates;
	TestTrue(
		TEXT("Authored states can be expanded into an addressable runtime array"),
		RegionSubsystem->BuildAuthoredRegionStates(/*FallbackState=*/3, InitialRegionStates));
	TestEqual(TEXT("Initial state array matches the highest SeedIndex"), InitialRegionStates.Num(), 8);
	TestEqual(TEXT("Region 2 keeps its authored biome state"), InitialRegionStates[2], static_cast<uint8>(1));
	TestEqual(TEXT("Sparse regions retain the caller's legacy fallback"), InitialRegionStates[4], static_cast<uint8>(3));
	TestEqual(TEXT("Region 7 keeps its authored biome state"), InitialRegionStates[7], static_cast<uint8>(5));

	AGP_GameState* GameState = TestWorld->SpawnActor<AGP_GameState>();
	if (TestNotNull(TEXT("Spawned authoritative GameState for array initialization"), GameState))
	{
		GameState->InitRegionStatesFromArray(InitialRegionStates);
		TestEqual(TEXT("GameState keeps the authored region count"), GameState->GetRegionCount(), 8);
		TestEqual(TEXT("GameState keeps an authored low-index state"), GameState->GetRegionState(2), static_cast<uint8>(1));
		TestEqual(TEXT("GameState keeps a sparse fallback state"), GameState->GetRegionState(4), static_cast<uint8>(3));
		TestEqual(TEXT("GameState keeps an authored high-index state"), GameState->GetRegionState(7), static_cast<uint8>(5));
	}

	int32 ParsedRegionId = INDEX_NONE;
	TestTrue(
		TEXT("Legacy actor labels provide a numeric suffix fallback"),
		UGP_RegionSpatialSubsystem::TryParseTrailingRegionId(TEXT("RegionSeed_14"), ParsedRegionId));
	TestEqual(TEXT("Fallback parser preserves the authored suffix"), ParsedRegionId, 14);
	TestFalse(
		TEXT("Fallback parser rejects labels without a numeric suffix"),
		UGP_RegionSpatialSubsystem::TryParseTrailingRegionId(TEXT("RegionSeed_Invalid"), ParsedRegionId));

	TestWorld->DestroyWorld(false);
	return true;
}

#endif
