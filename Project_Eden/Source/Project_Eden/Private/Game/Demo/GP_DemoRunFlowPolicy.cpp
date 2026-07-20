#include "Game/Demo/GP_DemoRunFlowPolicy.h"

namespace
{
	constexpr int32 RequiredRoutePointCount = 5;
	constexpr float MinimumRadiusStepFraction = 0.055f;
	constexpr float MinimumDirectionAlignment = 0.0f;

	struct FRouteCandidate
	{
		FGPDemoRegionSeedPoint Seed;
		float Radius = 0.0f;
	};

	bool IsUsableSeedPoint(const FGPDemoRegionSeedPoint& Point)
	{
		return Point.RegionId >= 0
			&& !Point.WorldLocation.ContainsNaN()
			&& FMath::IsFinite(Point.WorldLocation.X)
			&& FMath::IsFinite(Point.WorldLocation.Y)
			&& FMath::IsFinite(Point.WorldLocation.Z);
	}

	void NormalizeSeedPoints(
		const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
		TArray<FGPDemoRegionSeedPoint>& OutPoints)
	{
		TMap<int32, FGPDemoRegionSeedPoint> UniquePoints;
		for (const FGPDemoRegionSeedPoint& Point : SeedPoints)
		{
			if (!IsUsableSeedPoint(Point))
			{
				continue;
			}

			FGPDemoRegionSeedPoint* Existing = UniquePoints.Find(Point.RegionId);
			const bool bLocationSortsFirst = Existing
				&& (Point.WorldLocation.X < Existing->WorldLocation.X
					|| (Point.WorldLocation.X == Existing->WorldLocation.X
						&& (Point.WorldLocation.Y < Existing->WorldLocation.Y
							|| (Point.WorldLocation.Y == Existing->WorldLocation.Y
								&& Point.WorldLocation.Z < Existing->WorldLocation.Z))));
			if (!Existing)
			{
				UniquePoints.Add(Point.RegionId, Point);
			}
			else if (bLocationSortsFirst)
			{
				// Duplicate ids resolve by coordinates so actor iteration order cannot alter a run.
				*Existing = Point;
			}
		}

		UniquePoints.GenerateValueArray(OutPoints);
		OutPoints.Sort([](const FGPDemoRegionSeedPoint& Left, const FGPDemoRegionSeedPoint& Right)
		{
			// Sorting before every random choice makes results independent from actor iteration order.
			return Left.RegionId < Right.RegionId;
		});
	}

	FGPDemoRegionSeedPoint ResolveCenterSeed(const TArray<FGPDemoRegionSeedPoint>& Points)
	{
		FVector2D Centroid = FVector2D::ZeroVector;
		for (const FGPDemoRegionSeedPoint& Point : Points)
		{
			Centroid += FVector2D(Point.WorldLocation.X, Point.WorldLocation.Y);
		}
		Centroid /= FMath::Max(1, Points.Num());

		const FGPDemoRegionSeedPoint* BestPoint = nullptr;
		double BestDistanceSquared = TNumericLimits<double>::Max();
		for (const FGPDemoRegionSeedPoint& Point : Points)
		{
			const double DistanceSquared = FVector2D::DistSquared(
				FVector2D(Point.WorldLocation.X, Point.WorldLocation.Y),
				Centroid);
			if (!BestPoint
				|| DistanceSquared < BestDistanceSquared
				|| (FMath::IsNearlyEqual(DistanceSquared, BestDistanceSquared) && Point.RegionId < BestPoint->RegionId))
			{
				BestPoint = &Point;
				BestDistanceSquared = DistanceSquared;
			}
		}
		return BestPoint ? *BestPoint : FGPDemoRegionSeedPoint();
	}

	void BuildCandidates(
		const TArray<FGPDemoRegionSeedPoint>& Points,
		const FGPDemoRegionSeedPoint& CenterSeed,
		TArray<FRouteCandidate>& OutCandidates,
		float& OutMaximumRadius)
	{
		OutMaximumRadius = 0.0f;
		for (const FGPDemoRegionSeedPoint& Point : Points)
		{
			if (Point.RegionId == CenterSeed.RegionId)
			{
				continue;
			}

			FRouteCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
			Candidate.Seed = Point;
			Candidate.Radius = FVector::Dist2D(Point.WorldLocation, CenterSeed.WorldLocation);
			OutMaximumRadius = FMath::Max(OutMaximumRadius, Candidate.Radius);
		}

		OutCandidates.Sort([](const FRouteCandidate& Left, const FRouteCandidate& Right)
		{
			if (!FMath::IsNearlyEqual(Left.Radius, Right.Radius))
			{
				return Left.Radius > Right.Radius;
			}
			return Left.Seed.RegionId < Right.Seed.RegionId;
		});
	}

	void BuildPeripheralOrder(
		const TArray<FRouteCandidate>& Candidates,
		int32 RunSeed,
		TArray<int32>& OutRegionIds)
	{
		const int32 CandidateCount = FMath::Clamp(
			FMath::CeilToInt(static_cast<float>(Candidates.Num()) * 0.35f),
			2,
			Candidates.Num());
		for (int32 Index = 0; Index < CandidateCount; ++Index)
		{
			OutRegionIds.Add(Candidates[Index].Seed.RegionId);
		}

		FRandomStream PeripheralStream(RunSeed ^ 0x4F1BBCDC);
		for (int32 Index = OutRegionIds.Num() - 1; Index > 0; --Index)
		{
			OutRegionIds.Swap(Index, PeripheralStream.RandRange(0, Index));
		}
	}

	const FRouteCandidate* FindCandidateByRegion(
		const TArray<FRouteCandidate>& Candidates,
		int32 RegionId)
	{
		return Candidates.FindByPredicate([RegionId](const FRouteCandidate& Candidate)
		{
			return Candidate.Seed.RegionId == RegionId;
		});
	}

	bool IsInwardCandidateEligible(
		const FRouteCandidate& Candidate,
		int32 OuterRegionId,
		const FVector& CenterLocation,
		const FVector& OutwardDirection,
		float MaximumAllowedRadius)
	{
		if (Candidate.Seed.RegionId == OuterRegionId
			|| Candidate.Radius <= KINDA_SMALL_NUMBER
			|| Candidate.Radius > MaximumAllowedRadius)
		{
			return false;
		}

		const FVector CandidateDirection =
			(Candidate.Seed.WorldLocation - CenterLocation).GetSafeNormal2D();
		return FVector::DotProduct(OutwardDirection, CandidateDirection) >= MinimumDirectionAlignment;
	}

	float ScoreInwardCandidate(
		const FRouteCandidate& Candidate,
		const FVector& CenterLocation,
		const FVector& OutwardDirection,
		float OuterRadius,
		float DesiredOuterRadiusFraction,
		int32 RunSeed,
		int32 OuterRegionId,
		int32 StageIndex)
	{
		const FVector CandidateDirection =
			(Candidate.Seed.WorldLocation - CenterLocation).GetSafeNormal2D();
		const float DirectionAlignment = FVector::DotProduct(OutwardDirection, CandidateDirection);
		const float RadiusError = FMath::Abs(
			Candidate.Radius - OuterRadius * DesiredOuterRadiusFraction)
			/ FMath::Max(1.0f, OuterRadius);
		const float DirectionPenalty = (1.0f - DirectionAlignment) * 0.42f;

		// Each candidate gets an independent domain hash so loop ordering cannot change seeded tie breaks.
		const uint32 StageSalt = 0x632BE5ABu
			+ static_cast<uint32>(StageIndex) * 0x1B873593u;
		const uint32 MixedSeed = HashCombineFast(
			HashCombineFast(static_cast<uint32>(RunSeed), StageSalt),
			HashCombineFast(
				static_cast<uint32>(OuterRegionId),
				static_cast<uint32>(Candidate.Seed.RegionId)));
		const float StableTieBreak = static_cast<float>(MixedSeed & 0xFFFFu) / 65535.0f * 0.035f;
		return RadiusError * 2.5f + DirectionPenalty + StableTieBreak;
	}

	bool SelectInwardSequence(
		const TArray<FRouteCandidate>& Candidates,
		const FRouteCandidate& OuterCandidate,
		const FVector& CenterLocation,
		const FVector& OutwardDirection,
		int32 RunSeed,
		TArray<const FRouteCandidate*>& OutSequence)
	{
		OutSequence.Reset();
		const float OuterRadius = OuterCandidate.Radius;
		const float MinimumRadiusStep = FMath::Max(100.0f, OuterRadius * MinimumRadiusStepFraction);
		const float DesiredFractions[] = {0.78f, 0.54f, 0.30f};
		const FRouteCandidate* BestSequence[3] = {nullptr, nullptr, nullptr};
		float BestScore = TNumericLimits<float>::Max();
		for (const FRouteCandidate& OpeningCandidate : Candidates)
		{
			if (!IsInwardCandidateEligible(
				OpeningCandidate,
				OuterCandidate.Seed.RegionId,
				CenterLocation,
				OutwardDirection,
				OuterRadius - MinimumRadiusStep))
			{
				continue;
			}

			for (const FRouteCandidate& PressureCandidate : Candidates)
			{
				if (PressureCandidate.Seed.RegionId == OpeningCandidate.Seed.RegionId
					|| !IsInwardCandidateEligible(
						PressureCandidate,
						OuterCandidate.Seed.RegionId,
						CenterLocation,
						OutwardDirection,
						OpeningCandidate.Radius - MinimumRadiusStep))
				{
					continue;
				}

				for (const FRouteCandidate& RewardCandidate : Candidates)
				{
					if (RewardCandidate.Seed.RegionId == OpeningCandidate.Seed.RegionId
						|| RewardCandidate.Seed.RegionId == PressureCandidate.Seed.RegionId
						|| !IsInwardCandidateEligible(
							RewardCandidate,
							OuterCandidate.Seed.RegionId,
							CenterLocation,
							OutwardDirection,
							PressureCandidate.Radius - MinimumRadiusStep))
					{
						continue;
					}

					float Score = ScoreInwardCandidate(
						OpeningCandidate,
						CenterLocation,
						OutwardDirection,
						OuterRadius,
						DesiredFractions[0],
						RunSeed,
						OuterCandidate.Seed.RegionId,
						0);
					Score += ScoreInwardCandidate(
						PressureCandidate,
						CenterLocation,
						OutwardDirection,
						OuterRadius,
						DesiredFractions[1],
						RunSeed,
						OuterCandidate.Seed.RegionId,
						1);
					Score += ScoreInwardCandidate(
						RewardCandidate,
						CenterLocation,
						OutwardDirection,
						OuterRadius,
						DesiredFractions[2],
						RunSeed,
						OuterCandidate.Seed.RegionId,
						2);
					const float TravelDistance =
						FVector::Dist2D(OuterCandidate.Seed.WorldLocation, OpeningCandidate.Seed.WorldLocation)
						+ FVector::Dist2D(OpeningCandidate.Seed.WorldLocation, PressureCandidate.Seed.WorldLocation)
						+ FVector::Dist2D(PressureCandidate.Seed.WorldLocation, RewardCandidate.Seed.WorldLocation);
					// A small path-length term selects readable nearby beats when radial scores are comparable.
					Score += TravelDistance / FMath::Max(1.0f, OuterRadius) * 0.16f;
					if (!BestSequence[0] || Score < BestScore)
					{
						BestSequence[0] = &OpeningCandidate;
						BestSequence[1] = &PressureCandidate;
						BestSequence[2] = &RewardCandidate;
						BestScore = Score;
					}
				}
			}
		}

		if (!BestSequence[0])
		{
			return false;
		}
		OutSequence.Append(BestSequence, UE_ARRAY_COUNT(BestSequence));
		return true;
	}

	void AddRoutePoint(
		FGPDemoRunRoute& Route,
		EGPDemoRouteRole Role,
		const FRouteCandidate& Candidate,
		float MaximumRadius)
	{
		FGPDemoRoutePoint& Point = Route.Points.AddDefaulted_GetRef();
		Point.Role = Role;
		Point.RegionId = Candidate.Seed.RegionId;
		Point.WorldLocation = Candidate.Seed.WorldLocation;
		Point.NormalizedCenterDistance = Candidate.Radius / FMath::Max(1.0f, MaximumRadius);
	}

	bool BuildRouteInternal(
		const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
		int32 RunSeed,
		int32 ForcedOuterRegionId,
		FGPDemoRunRoute& OutRoute)
	{
		OutRoute = FGPDemoRunRoute();
		OutRoute.RunSeed = RunSeed;

		TArray<FGPDemoRegionSeedPoint> Points;
		NormalizeSeedPoints(SeedPoints, Points);
		if (Points.Num() < RequiredRoutePointCount)
		{
			return false;
		}

		const FGPDemoRegionSeedPoint CenterSeed = ResolveCenterSeed(Points);
		if (CenterSeed.RegionId == INDEX_NONE)
		{
			return false;
		}

		TArray<FRouteCandidate> Candidates;
		float MaximumRadius = 0.0f;
		BuildCandidates(Points, CenterSeed, Candidates, MaximumRadius);
		if (Candidates.Num() < RequiredRoutePointCount - 1 || MaximumRadius <= KINDA_SMALL_NUMBER)
		{
			return false;
		}

		BuildPeripheralOrder(Candidates, RunSeed, OutRoute.PeripheralCandidateRegionIds);
		const int32 OuterRegionId = ForcedOuterRegionId != INDEX_NONE
			? ForcedOuterRegionId
			: (OutRoute.PeripheralCandidateRegionIds.IsEmpty()
				? INDEX_NONE
				: OutRoute.PeripheralCandidateRegionIds[0]);
		const FRouteCandidate* OuterCandidate = FindCandidateByRegion(Candidates, OuterRegionId);
		if (!OuterCandidate || !OutRoute.PeripheralCandidateRegionIds.Contains(OuterRegionId))
		{
			return false;
		}

		OutRoute.CenterLocation = CenterSeed.WorldLocation;
		AddRoutePoint(OutRoute, EGPDemoRouteRole::OuterStart, *OuterCandidate, MaximumRadius);

		const FVector OutwardDirection =
			(OuterCandidate->Seed.WorldLocation - CenterSeed.WorldLocation).GetSafeNormal2D();
		const EGPDemoRouteRole Roles[] =
		{
			EGPDemoRouteRole::OpeningObjective,
			EGPDemoRouteRole::PressureObjective,
			EGPDemoRouteRole::InnerReward
		};
		TArray<const FRouteCandidate*> SelectedSequence;
		if (!SelectInwardSequence(
			Candidates,
			*OuterCandidate,
			CenterSeed.WorldLocation,
			OutwardDirection,
			RunSeed,
			SelectedSequence))
		{
			OutRoute.Points.Reset();
			return false;
		}

		for (int32 StageIndex = 0; StageIndex < UE_ARRAY_COUNT(Roles); ++StageIndex)
		{
			AddRoutePoint(OutRoute, Roles[StageIndex], *SelectedSequence[StageIndex], MaximumRadius);
		}

		FRouteCandidate CenterCandidate;
		CenterCandidate.Seed = CenterSeed;
		CenterCandidate.Radius = 0.0f;
		AddRoutePoint(OutRoute, EGPDemoRouteRole::CenterBoss, CenterCandidate, MaximumRadius);
		return OutRoute.IsValid() && GPDemoRunFlowPolicy::IsStrictlyInward(OutRoute);
	}
}

bool FGPDemoRunRoute::IsValid() const
{
	if (Points.Num() != RequiredRoutePointCount)
	{
		return false;
	}

	const EGPDemoRouteRole ExpectedRoles[] =
	{
		EGPDemoRouteRole::OuterStart,
		EGPDemoRouteRole::OpeningObjective,
		EGPDemoRouteRole::PressureObjective,
		EGPDemoRouteRole::InnerReward,
		EGPDemoRouteRole::CenterBoss
	};
	for (int32 RoleIndex = 0; RoleIndex < RequiredRoutePointCount; ++RoleIndex)
	{
		if (Points[RoleIndex].Role != ExpectedRoles[RoleIndex]
			|| Points[RoleIndex].RegionId < 0)
		{
			return false;
		}
	}
	return true;
}

const FGPDemoRoutePoint* FGPDemoRunRoute::FindPoint(EGPDemoRouteRole Role) const
{
	return Points.FindByPredicate([Role](const FGPDemoRoutePoint& Point)
	{
		return Point.Role == Role;
	});
}

bool GPDemoRunFlowPolicy::BuildInwardRoute(
	const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
	int32 RunSeed,
	FGPDemoRunRoute& OutRoute)
{
	FGPDemoRunRoute CandidateProbe;
	BuildRouteInternal(SeedPoints, RunSeed, INDEX_NONE, CandidateProbe);
	const TArray<int32> CandidateOrder = CandidateProbe.PeripheralCandidateRegionIds;
	TArray<int32> ValidCandidateOrder;
	FGPDemoRunRoute FirstValidRoute;
	for (const int32 CandidateRegionId : CandidateOrder)
	{
		FGPDemoRunRoute CandidateRoute;
		if (BuildRouteInternal(SeedPoints, RunSeed, CandidateRegionId, CandidateRoute))
		{
			if (ValidCandidateOrder.IsEmpty())
			{
				FirstValidRoute = CandidateRoute;
			}
			ValidCandidateOrder.Add(CandidateRegionId);
		}
	}

	if (ValidCandidateOrder.IsEmpty())
	{
		OutRoute = MoveTemp(CandidateProbe);
		OutRoute.Points.Reset();
		return false;
	}

	FirstValidRoute.PeripheralCandidateRegionIds = MoveTemp(ValidCandidateOrder);
	OutRoute = MoveTemp(FirstValidRoute);
	return true;
}

bool GPDemoRunFlowPolicy::BuildInwardRouteFromOuterRegion(
	const TArray<FGPDemoRegionSeedPoint>& SeedPoints,
	int32 RunSeed,
	int32 OuterRegionId,
	FGPDemoRunRoute& OutRoute)
{
	FGPDemoRunRoute BaseRoute;
	if (!BuildInwardRoute(SeedPoints, RunSeed, BaseRoute)
		|| !BaseRoute.PeripheralCandidateRegionIds.Contains(OuterRegionId)
		|| !BuildRouteInternal(SeedPoints, RunSeed, OuterRegionId, OutRoute))
	{
		OutRoute = FGPDemoRunRoute();
		return false;
	}
	OutRoute.PeripheralCandidateRegionIds = MoveTemp(BaseRoute.PeripheralCandidateRegionIds);
	return true;
}

bool GPDemoRunFlowPolicy::IsStrictlyInward(const FGPDemoRunRoute& Route)
{
	if (!Route.IsValid())
	{
		return false;
	}

	TSet<int32> UsedRegions;
	double PreviousDistance = TNumericLimits<double>::Max();
	for (const FGPDemoRoutePoint& Point : Route.Points)
	{
		const double Distance = FVector::Dist2D(Point.WorldLocation, Route.CenterLocation);
		if (UsedRegions.Contains(Point.RegionId)
			|| !FMath::IsFinite(Distance)
			|| !FMath::IsFinite(Point.NormalizedCenterDistance)
			|| Distance >= PreviousDistance - KINDA_SMALL_NUMBER)
		{
			return false;
		}
		UsedRegions.Add(Point.RegionId);
		PreviousDistance = Distance;
	}
	return true;
}
