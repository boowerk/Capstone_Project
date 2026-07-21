#if WITH_DEV_AUTOMATION_TESTS

#include "Game/Demo/GP_DemoRunFlowPolicy.h"

#include "Algo/Reverse.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	FGPDemoRegionSeedPoint MakeSeedPoint(int32 RegionId, const FVector& WorldLocation)
	{
		FGPDemoRegionSeedPoint Point;
		Point.RegionId = RegionId;
		Point.WorldLocation = WorldLocation;
		return Point;
	}

	TArray<FGPDemoRegionSeedPoint> BuildRouteFixture()
	{
		// Multiple candidates at each ring exercise seeded variation without changing the five authored roles.
		return {
			MakeSeedPoint(0, FVector(0.0f, 0.0f, 100.0f)),
			MakeSeedPoint(1, FVector(2800.0f, 900.0f, 100.0f)),
			MakeSeedPoint(2, FVector(3000.0f, -500.0f, 100.0f)),
			MakeSeedPoint(3, FVector(5200.0f, 1200.0f, 100.0f)),
			MakeSeedPoint(4, FVector(5500.0f, -700.0f, 100.0f)),
			MakeSeedPoint(5, FVector(7400.0f, 1600.0f, 100.0f)),
			MakeSeedPoint(6, FVector(7700.0f, -900.0f, 100.0f)),
			MakeSeedPoint(7, FVector(9800.0f, 1800.0f, 100.0f)),
			MakeSeedPoint(8, FVector(10100.0f, -1300.0f, 100.0f)),
			MakeSeedPoint(9, FVector(-9700.0f, 2100.0f, 100.0f)),
			MakeSeedPoint(10, FVector(-7600.0f, 1500.0f, 100.0f)),
			MakeSeedPoint(11, FVector(-5300.0f, 900.0f, 100.0f)),
			MakeSeedPoint(12, FVector(-2900.0f, 500.0f, 100.0f)),
			MakeSeedPoint(13, FVector(800.0f, -3100.0f, 100.0f)),
			MakeSeedPoint(14, FVector(-1200.0f, 5600.0f, 100.0f))
		};
	}

	bool RoutesMatch(const FGPDemoRunRoute& Left, const FGPDemoRunRoute& Right)
	{
		if (Left.RunSeed != Right.RunSeed
			|| !Left.CenterLocation.Equals(Right.CenterLocation)
			|| Left.PeripheralCandidateRegionIds != Right.PeripheralCandidateRegionIds
			|| Left.Points.Num() != Right.Points.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < Left.Points.Num(); ++Index)
		{
			if (Left.Points[Index].Role != Right.Points[Index].Role
				|| Left.Points[Index].RegionId != Right.Points[Index].RegionId
				|| !Left.Points[Index].WorldLocation.Equals(Right.Points[Index].WorldLocation)
				|| !FMath::IsNearlyEqual(
					Left.Points[Index].NormalizedCenterDistance,
					Right.Points[Index].NormalizedCenterDistance))
			{
				return false;
			}
		}
		return true;
	}

	bool HasDemoRouteClassInHierarchy(const AActor& Actor, const FName RequiredClassName)
	{
		for (const UClass* ActorClass = Actor.GetClass(); ActorClass; ActorClass = ActorClass->GetSuperClass())
		{
			if (ActorClass->GetFName() == RequiredClassName)
			{
				return true;
			}
		}
		return false;
	}

	bool LoadProductionSeedPoints(TArray<FGPDemoRegionSeedPoint>& OutPoints)
	{
		UPackage* MapPackage = LoadPackage(nullptr, TEXT("/Game/Maps/MainMap/L_LandscapeMap"), LOAD_None);
		UWorld* World = MapPackage ? UWorld::FindWorldInPackage(MapPackage) : nullptr;
		if (!World || !World->PersistentLevel)
		{
			return false;
		}

		const FName RegionSeedClassName(TEXT("BP_RegionSeed_C"));
		for (AActor* Actor : World->PersistentLevel->Actors)
		{
			if (!IsValid(Actor) || !HasDemoRouteClassInHierarchy(*Actor, RegionSeedClassName))
			{
				continue;
			}

			const FIntProperty* SeedIndexProperty =
				FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("SeedIndex"));
			if (SeedIndexProperty)
			{
				OutPoints.Add(MakeSeedPoint(
					SeedIndexProperty->GetPropertyValue_InContainer(Actor),
					Actor->GetActorLocation()));
			}
		}
		return OutPoints.Num() == 15;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPDemoRunRouteDeterminismTest,
	"ProjectEden.Game.DemoFlow.RouteDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPDemoRunRouteDeterminismTest::RunTest(const FString& Parameters)
{
	const TArray<FGPDemoRegionSeedPoint> SeedPoints = BuildRouteFixture();
	FGPDemoRunRoute FirstRoute;
	FGPDemoRunRoute RepeatedRoute;
	TestTrue(TEXT("The fixture produces a route"), GPDemoRunFlowPolicy::BuildInwardRoute(SeedPoints, 12031, FirstRoute));
	TestTrue(TEXT("The same seed produces a second route"), GPDemoRunFlowPolicy::BuildInwardRoute(SeedPoints, 12031, RepeatedRoute));
	TestTrue(TEXT("The same run seed reproduces every route point"), RoutesMatch(FirstRoute, RepeatedRoute));
	TestTrue(TEXT("Every route point moves strictly toward the center"), GPDemoRunFlowPolicy::IsStrictlyInward(FirstRoute));
	TestEqual(TEXT("The route contains the five fixed demo roles"), FirstRoute.Points.Num(), 5);

	TArray<FGPDemoRegionSeedPoint> ReversedPoints = SeedPoints;
	Algo::Reverse(ReversedPoints);
	FGPDemoRunRoute ReorderedRoute;
	TestTrue(TEXT("Reordered seed input remains usable"), GPDemoRunFlowPolicy::BuildInwardRoute(ReversedPoints, 12031, ReorderedRoute));
	TestTrue(TEXT("Actor iteration order cannot alter the seeded route"), RoutesMatch(FirstRoute, ReorderedRoute));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPDemoRunRouteVariationTest,
	"ProjectEden.Game.DemoFlow.RouteVariation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPDemoRunRouteVariationTest::RunTest(const FString& Parameters)
{
	const TArray<FGPDemoRegionSeedPoint> SeedPoints = BuildRouteFixture();
	TSet<int32> OuterRegions;
	for (int32 RunSeed = 1; RunSeed <= 32; ++RunSeed)
	{
		FGPDemoRunRoute Route;
		if (!TestTrue(
			*FString::Printf(TEXT("Seed %d produces a valid route"), RunSeed),
			GPDemoRunFlowPolicy::BuildInwardRoute(SeedPoints, RunSeed, Route)))
		{
			continue;
		}
		TestTrue(TEXT("Every varied route remains strictly inward"), GPDemoRunFlowPolicy::IsStrictlyInward(Route));
		OuterRegions.Add(Route.Points[0].RegionId);
	}
	TestTrue(TEXT("Thirty-two runs use at least three different outer starts"), OuterRegions.Num() >= 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPDemoRunRouteFallbackTest,
	"ProjectEden.Game.DemoFlow.RouteFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPDemoRunRouteFallbackTest::RunTest(const FString& Parameters)
{
	const TArray<FGPDemoRegionSeedPoint> SeedPoints = BuildRouteFixture();
	FGPDemoRunRoute InitialRoute;
	TestTrue(TEXT("Initial route exposes peripheral fallbacks"), GPDemoRunFlowPolicy::BuildInwardRoute(SeedPoints, 77, InitialRoute));
	TestTrue(TEXT("At least two outer candidates are available"), InitialRoute.PeripheralCandidateRegionIds.Num() >= 2);

	if (InitialRoute.PeripheralCandidateRegionIds.Num() >= 2)
	{
		FGPDemoRunRoute AlternateRoute;
		const int32 AlternateOuterRegion = InitialRoute.PeripheralCandidateRegionIds[1];
		const bool bBuiltAlternateRoute = TestTrue(
			TEXT("A safe fallback can rebuild the route around the next authored edge"),
			GPDemoRunFlowPolicy::BuildInwardRouteFromOuterRegion(
				SeedPoints,
				77,
				AlternateOuterRegion,
				AlternateRoute));
		if (bBuiltAlternateRoute && !AlternateRoute.Points.IsEmpty())
		{
			TestEqual(TEXT("The forced fallback becomes the outer start"), AlternateRoute.Points[0].RegionId, AlternateOuterRegion);
			TestTrue(TEXT("The fallback route still moves inward"), GPDemoRunFlowPolicy::IsStrictlyInward(AlternateRoute));
		}
	}

	TArray<FGPDemoRegionSeedPoint> TooFewPoints;
	TooFewPoints.Append(SeedPoints.GetData(), 4);
	FGPDemoRunRoute InvalidRoute;
	TestFalse(TEXT("Fewer than five authored points fail closed"), GPDemoRunFlowPolicy::BuildInwardRoute(TooFewPoints, 77, InvalidRoute));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FGPDemoRunProductionRouteTest,
	"ProjectEden.Game.DemoFlow.ProductionMapRoutes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGPDemoRunProductionRouteTest::RunTest(const FString& Parameters)
{
	TArray<FGPDemoRegionSeedPoint> ProductionSeeds;
	if (!TestTrue(TEXT("The production landscape exposes all 15 route seeds"), LoadProductionSeedPoints(ProductionSeeds)))
	{
		return false;
	}
	ProductionSeeds.Sort([](const FGPDemoRegionSeedPoint& Left, const FGPDemoRegionSeedPoint& Right)
	{
		return Left.RegionId < Right.RegionId;
	});
	for (const FGPDemoRegionSeedPoint& SeedPoint : ProductionSeeds)
	{
		// Keep production coordinates visible in failed automation logs so route regressions are actionable.
		AddInfo(FString::Printf(
			TEXT("Production region %d is at %s"),
			SeedPoint.RegionId,
			*SeedPoint.WorldLocation.ToCompactString()));
	}

	TSet<int32> OuterRegions;
	for (int32 RunSeed = 1; RunSeed <= 128; ++RunSeed)
	{
		FGPDemoRunRoute Route;
		if (!TestTrue(
			*FString::Printf(TEXT("Production seed %d builds an inward route"), RunSeed),
			GPDemoRunFlowPolicy::BuildInwardRoute(ProductionSeeds, RunSeed, Route)))
		{
			continue;
		}

		OuterRegions.Add(Route.Points[0].RegionId);
		TestTrue(TEXT("Production route distances decrease"), GPDemoRunFlowPolicy::IsStrictlyInward(Route));
		for (const int32 PeripheralRegionId : Route.PeripheralCandidateRegionIds)
		{
			FGPDemoRunRoute FallbackRoute;
			TestTrue(
				*FString::Printf(
					TEXT("Seed %d supports peripheral region %d"),
					RunSeed,
					PeripheralRegionId),
				GPDemoRunFlowPolicy::BuildInwardRouteFromOuterRegion(
					ProductionSeeds,
					RunSeed,
					PeripheralRegionId,
					FallbackRoute));
		}
	}
	TestTrue(TEXT("Production runs vary across at least three outer regions"), OuterRegions.Num() >= 3);
	return true;
}

#endif
