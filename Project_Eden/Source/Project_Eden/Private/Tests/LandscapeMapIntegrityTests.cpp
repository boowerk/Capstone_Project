#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Components/ActorComponent.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerStart.h"
#include "UObject/UnrealType.h"
#include "UObject/Package.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	constexpr int32 ExpectedLandscapeRegionCount = 15;
	constexpr float MinimumPlayablePlayerStartZ = 800.0f;
	constexpr double MinimumNavigationXYCoverage = 0.9;

	bool HasClassInHierarchy(const AActor& Actor, const FName RequiredClassName)
	{
		// Native and Blueprint subclasses both satisfy the map contract when their hierarchy owns the required role.
		for (const UClass* ActorClass = Actor.GetClass(); ActorClass != nullptr; ActorClass = ActorClass->GetSuperClass())
		{
			if (ActorClass->GetFName() == RequiredClassName)
			{
				return true;
			}
		}

		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPLandscapeMapIntegrityTest,
	"ProjectEden.Game.LandscapeMap.Integrity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPLandscapeMapIntegrityTest::RunTest(const FString& Parameters)
{
	constexpr TCHAR MapPackagePath[] = TEXT("/Game/Maps/MainMap/L_LandscapeMap");
	const FName TestEnvironmentTag(TEXT("GP.TestEnvironment"));
	const FName LandscapeClassName(TEXT("Landscape"));
	const FName LandscapeProxyClassName(TEXT("LandscapeStreamingProxy"));
	const FName LandscapeComponentClassName(TEXT("LandscapeComponent"));
	const FName LandscapeCollisionClassName(TEXT("LandscapeHeightfieldCollisionComponent"));
	const FName NavigationBoundsClassName(TEXT("NavMeshBoundsVolume"));
	const FName RegionEventDirectorClassName(TEXT("GP_RegionEventDirector"));
	const FName RegionSeedClassName(TEXT("BP_RegionSeed_C"));
	UPackage* MapPackage = LoadPackage(nullptr, MapPackagePath, LOAD_None);
	TestNotNull(TEXT("L_LandscapeMap package loads"), MapPackage);

	UWorld* World = MapPackage ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
	TestNotNull(TEXT("L_LandscapeMap contains a world"), World);
	if (!World || !World->PersistentLevel)
	{
		return false;
	}

	int32 LandscapeActorCount = 0;
	int32 LandscapeComponentCount = 0;
	int32 LandscapeCollisionCount = 0;
	int32 TestEnvironmentActorCount = 0;
	int32 PlayerStartCount = 0;
	int32 NavigationBoundsCount = 0;
	int32 RegionSeedActorCount = 0;
	int32 RegionEventDirectorCount = 0;
	float LowestPlayerStartZ = TNumericLimits<float>::Max();
	FBox LandscapeBounds(EForceInit::ForceInit);
	FBox NavigationBounds(EForceInit::ForceInit);
	TSet<int32> AddressableRegionIndices;
	for (AActor* Actor : World->PersistentLevel->Actors)
	{
		if (!IsValid(Actor))
		{
			continue;
		}

		if (Actor->Tags.Contains(TestEnvironmentTag))
		{
			++TestEnvironmentActorCount;
		}

		const FName ActorClassName = Actor->GetClass()->GetFName();
		if (ActorClassName == LandscapeClassName || ActorClassName == LandscapeProxyClassName)
		{
			++LandscapeActorCount;
			// Serialized component bounds let the test compare authored navigation coverage without starting PIE.
			LandscapeBounds += Actor->GetComponentsBoundingBox(true);
		}

		if (Actor->IsA<APlayerStart>())
		{
			++PlayerStartCount;
			LowestPlayerStartZ = FMath::Min(LowestPlayerStartZ, Actor->GetActorLocation().Z);
		}

		if (HasClassInHierarchy(*Actor, NavigationBoundsClassName))
		{
			++NavigationBoundsCount;
			// All authored navigation volumes are combined because large landscapes may intentionally use tiled bounds.
			NavigationBounds += Actor->GetComponentsBoundingBox(true);
		}

		if (HasClassInHierarchy(*Actor, RegionEventDirectorClassName))
		{
			++RegionEventDirectorCount;
		}

		if (HasClassInHierarchy(*Actor, RegionSeedClassName))
		{
			++RegionSeedActorCount;
			// SeedIndex is a Blueprint variable, so reflection keeps this editor test independent of the Blueprint type.
			if (const FIntProperty* SeedIndexProperty = FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("SeedIndex")))
			{
				AddressableRegionIndices.Add(SeedIndexProperty->GetPropertyValue_InContainer(Actor));
			}
		}

		// Class-name checks avoid adding a runtime Landscape module dependency solely for an editor integrity test.
		TInlineComponentArray<UActorComponent*> Components(Actor);
		for (const UActorComponent* Component : Components)
		{
			if (!IsValid(Component))
			{
				continue;
			}

			const FName ComponentClassName = Component->GetClass()->GetFName();
			if (ComponentClassName == LandscapeComponentClassName)
			{
				++LandscapeComponentCount;
			}
			else if (ComponentClassName == LandscapeCollisionClassName)
			{
				++LandscapeCollisionCount;
			}
		}
	}

	// These assertions catch the prior regression where a commandlet save retained the map but stripped sculpt data.
	TestTrue(TEXT("Sculpted Landscape actor remains present"), LandscapeActorCount > 0);
	TestTrue(TEXT("Sculpted Landscape components remain present"), LandscapeComponentCount > 0);
	TestTrue(TEXT("Landscape collision components remain present"), LandscapeCollisionCount > 0);
	TestEqual(TEXT("Production landscape contains no generated test-deck actors"), TestEnvironmentActorCount, 0);

	// Immediate-play infrastructure prevents players, AI, and corruption events from starting in an unusable world.
	TestTrue(TEXT("Production landscape contains a PlayerStart"), PlayerStartCount > 0);
	TestTrue(
		TEXT("Every PlayerStart is above the sculpted ground baseline"),
		PlayerStartCount > 0 && LowestPlayerStartZ >= MinimumPlayablePlayerStartZ);
	TestTrue(TEXT("Production landscape contains NavMesh bounds"), NavigationBoundsCount > 0);
	TestTrue(TEXT("Sculpted Landscape exposes valid world bounds"), LandscapeBounds.IsValid != 0);
	TestTrue(TEXT("NavMesh bounds expose valid world bounds"), NavigationBounds.IsValid != 0);

	if (LandscapeBounds.IsValid && NavigationBounds.IsValid)
	{
		const double LandscapeWidth = FMath::Max(1.0, static_cast<double>(LandscapeBounds.Max.X - LandscapeBounds.Min.X));
		const double LandscapeDepth = FMath::Max(1.0, static_cast<double>(LandscapeBounds.Max.Y - LandscapeBounds.Min.Y));
		const double OverlapWidth = FMath::Max(
			0.0,
			static_cast<double>(FMath::Min(LandscapeBounds.Max.X, NavigationBounds.Max.X)
				- FMath::Max(LandscapeBounds.Min.X, NavigationBounds.Min.X)));
		const double OverlapDepth = FMath::Max(
			0.0,
			static_cast<double>(FMath::Min(LandscapeBounds.Max.Y, NavigationBounds.Max.Y)
				- FMath::Max(LandscapeBounds.Min.Y, NavigationBounds.Min.Y)));
		const double CoverageX = OverlapWidth / LandscapeWidth;
		const double CoverageY = OverlapDepth / LandscapeDepth;

		// Report authored extents so a failed map edit is actionable from the automation log.
		AddInfo(FString::Printf(
			TEXT("Landscape bounds %s; NavMesh bounds %s; XY coverage %.3f x %.3f"),
			*LandscapeBounds.ToString(),
			*NavigationBounds.ToString(),
			CoverageX,
			CoverageY));
		TestTrue(TEXT("NavMesh bounds substantially cover Landscape X"), CoverageX >= MinimumNavigationXYCoverage);
		TestTrue(TEXT("NavMesh bounds substantially cover Landscape Y"), CoverageY >= MinimumNavigationXYCoverage);
		TestTrue(TEXT("NavMesh bounds include Landscape minimum Z"), NavigationBounds.Min.Z <= LandscapeBounds.Min.Z);
		TestTrue(TEXT("NavMesh bounds include Landscape maximum Z"), NavigationBounds.Max.Z >= LandscapeBounds.Max.Z);
	}

	TestEqual(TEXT("Landscape contains exactly 15 region seed actors"), RegionSeedActorCount, ExpectedLandscapeRegionCount);
	TestEqual(TEXT("Landscape exposes exactly 15 unique SeedIndex values"), AddressableRegionIndices.Num(), ExpectedLandscapeRegionCount);
	for (int32 ExpectedRegionId = 0; ExpectedRegionId < ExpectedLandscapeRegionCount; ++ExpectedRegionId)
	{
		// A contiguous address space is required by the replicated region-state and corruption arrays.
		TestTrue(
			*FString::Printf(TEXT("Region SeedIndex %d is addressable"), ExpectedRegionId),
			AddressableRegionIndices.Contains(ExpectedRegionId));
	}
	TestEqual(TEXT("Landscape contains one region event director"), RegionEventDirectorCount, 1);
	return true;
}

#endif
