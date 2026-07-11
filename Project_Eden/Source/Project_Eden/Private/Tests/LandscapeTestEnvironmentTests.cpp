#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/BoxComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Game/Corruption/GP_CorruptionTestZoneActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"
#include "Game/Testing/GP_LandscapeTestFloorActor.h"
#include "GameFramework/PlayerStart.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLandscapeTestEnvironmentMapContractTest,
	"ProjectEden.Game.LandscapeTestEnvironment.MapContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLandscapeTestEnvironmentMapContractTest::RunTest(const FString& Parameters)
{
	constexpr TCHAR MapPackagePath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
	const FName EnvironmentTag(TEXT("GP.TestEnvironment"));
	UPackage* MapPackage = LoadPackage(nullptr, MapPackagePath, LOAD_None);
	TestNotNull(TEXT("L_LandscapeMap package loads"), MapPackage);

	UWorld* World = MapPackage ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
	TestNotNull(TEXT("L_LandscapeMap contains a world"), World);
	if (!World || !World->PersistentLevel)
	{
		return false;
	}

	int32 PlayerStartCount = 0;
	int32 ManagedActorCount = 0;
	int32 CorruptionStationCount = 0;
	int32 EventStationCount = 0;
	int32 TestFloorCount = 0;
	int32 NavigationBoundsCount = 0;
	int32 InstructionSignCount = 0;
	TSet<int32> CorruptionValues;
	TSet<int32> EventRegionIds;
	TSet<FString> EventDataPaths;

	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->IsA<APlayerStart>())
		{
			++PlayerStartCount;
		}

		if (!Actor->Tags.Contains(EnvironmentTag))
		{
			continue;
		}

		++ManagedActorCount;
		if (AGP_CorruptionTestZoneActor* Station = Cast<AGP_CorruptionTestZoneActor>(Actor))
		{
			++CorruptionStationCount;
			CorruptionValues.Add(FMath::RoundToInt(Station->GetTestCorruption()));
			TestEqual(TEXT("Corruption station scope is world"), Station->GetTestScope(), EGPCorruptionTestScope::World);
			TestFalse(TEXT("Corruption station does not auto-apply on BeginPlay"), Station->IsApplyOnBeginPlayEnabled());
			TestTrue(TEXT("Corruption station applies on player overlap"), Station->IsApplyOnPlayerOverlapEnabled());
			TestTrue(TEXT("Corruption station pauses passive increase"), Station->ShouldPausePassiveIncrease());
		}
		else if (AGP_RegionEventTestTriggerActor* Trigger = Cast<AGP_RegionEventTestTriggerActor>(Actor))
		{
			++EventStationCount;
			EventRegionIds.Add(Trigger->GetTestRegionId());
			TestNotNull(TEXT("Event station has EventData"), Trigger->GetEventData());
			if (Trigger->GetEventData())
			{
				EventDataPaths.Add(Trigger->GetEventData()->GetPathName());
			}
			TestFalse(TEXT("Event station does not auto-trigger on BeginPlay"), Trigger->IsTriggerOnBeginPlay());
			TestTrue(TEXT("Event station triggers on player overlap"), Trigger->IsTriggerOnPlayerOverlap());
			TestFalse(TEXT("Event station has a visible title"), Trigger->GetStationTitle().IsEmpty());
		}
		else if (AGP_LandscapeTestFloorActor* Floor = Cast<AGP_LandscapeTestFloorActor>(Actor))
		{
			++TestFloorCount;
			TestNotNull(TEXT("Test floor has navigation collision"), Floor->GetNavigationFloor());
			if (Floor->GetNavigationFloor())
			{
				TestTrue(TEXT("Test floor collision blocks navigation geometry"), Floor->GetNavigationFloor()->IsNavigationRelevant());
			}
		}
		else if (Actor->IsA<ANavMeshBoundsVolume>())
		{
			++NavigationBoundsCount;
		}
		else if (Actor->Tags.Contains(TEXT("GP.Test.Instructions")))
		{
			++InstructionSignCount;
		}
	}

	TestEqual(TEXT("Existing PlayerStart remains unique"), PlayerStartCount, 1);
	TestEqual(TEXT("Managed test actor count is idempotent"), ManagedActorCount, 12);
	TestEqual(TEXT("Three corruption stations are placed"), CorruptionStationCount, 3);
	TestTrue(TEXT("Corruption values include 0"), CorruptionValues.Contains(0));
	TestTrue(TEXT("Corruption values include 50"), CorruptionValues.Contains(50));
	TestTrue(TEXT("Corruption values include 100"), CorruptionValues.Contains(100));
	TestEqual(TEXT("Four Region Event stations are placed"), EventStationCount, 4);
	for (int32 RegionId = 0; RegionId < 4; ++RegionId)
	{
		TestTrue(FString::Printf(TEXT("Event region %d exists"), RegionId), EventRegionIds.Contains(RegionId));
	}

	const FString ExpectedEventDataPaths[] =
	{
		TEXT("/Game/RegionEvents/Examples/DA_RE_Test_RedRift.DA_RE_Test_RedRift"),
		TEXT("/Game/RegionEvents/Examples/DA_RE_Test_CrystalCorruption.DA_RE_Test_CrystalCorruption"),
		TEXT("/Game/RegionEvents/Examples/DA_RE_Test_ShrineRuins.DA_RE_Test_ShrineRuins"),
		TEXT("/Game/RegionEvents/Examples/DA_RE_Test_StructureDefense.DA_RE_Test_StructureDefense"),
	};
	for (const FString& ExpectedPath : ExpectedEventDataPaths)
	{
		TestTrue(FString::Printf(TEXT("Event data %s is assigned"), *ExpectedPath), EventDataPaths.Contains(ExpectedPath));
	}

	TestEqual(TEXT("Three connected test floor pieces are placed"), TestFloorCount, 3);
	TestEqual(TEXT("One managed NavMesh bounds volume is placed"), NavigationBoundsCount, 1);
	TestEqual(TEXT("One instruction sign is placed"), InstructionSignCount, 1);
	return true;
}

#endif
