#include "Game/GP_EnemySpawnVolume.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/GP_GroundPlacement.h"

namespace GPEnemySpawnVolume
{
	constexpr int32 SpawnProjectionAttempts = 8;
	const FGPGroundPlacementSettings AuthoredPointSettings
	{
		FVector(300.0f, 300.0f, 300.0f),
		250.0f,
		SpawnProjectionAttempts
	};
	const FGPGroundPlacementSettings BossPointSettings
	{
		FVector(300.0f, 300.0f, 800.0f),
		250.0f,
		SpawnProjectionAttempts
	};
}

AGP_EnemySpawnVolume::AGP_EnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SpawnBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SpawnBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SpawnBox->SetGenerateOverlapEvents(true);
	SpawnBox->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnBox->ShapeColor = FColor(80, 200, 120);
}

void AGP_EnemySpawnVolume::BeginPlay()
{
	Super::BeginPlay();

	// Dynamically streamed village levels are also loaded locally for client-side
	// visual PCG. Their non-replicated gameplay actors report ROLE_Authority, so
	// net mode is the reliable guard against duplicate client encounter logic.
	if (GetNetMode() == NM_Client)
	{
		SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	SpawnBox->OnComponentBeginOverlap.AddDynamic(this, &AGP_EnemySpawnVolume::HandleBoxOverlap);
	SpawnBox->OnComponentEndOverlap.AddDynamic(this, &AGP_EnemySpawnVolume::HandleBoxEndOverlap);

	// Collect AGP_EnemySpawnMarker actors whose centers fall inside this box.
	TArray<AActor*> AllMarkers;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemySpawnMarker::StaticClass(), AllMarkers);
	for (AActor* Actor : AllMarkers)
	{
		if (AGP_EnemySpawnMarker* Marker = Cast<AGP_EnemySpawnMarker>(Actor))
		{
			if (Marker->GetLevel() != GetLevel())
			{
				continue;
			}

			// Use point-in-box check instead of overlap (overlap not ready at BeginPlay).
			const FVector Local =
				SpawnBox->GetComponentTransform().InverseTransformPositionNoScale(
					Marker->GetActorLocation());
			const FVector Extent = SpawnBox->GetScaledBoxExtent();
			if (FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z)
			{
				Markers.Add(Marker);
				UE_LOG(LogTemp, Log, TEXT("[GP_EnemySpawnVolume] Collected marker '%s'"), *Marker->GetName());
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("[GP_EnemySpawnVolume] Zone '%s' collected %d markers"), *GetName(), Markers.Num());

	CollectTaggedSpawnPoints();
}

void AGP_EnemySpawnVolume::Unlock()
{
	bUnlocked = true;
}

void AGP_EnemySpawnVolume::ConfigureRuntimeZone(
	FName InZoneId,
	EGPZoneStage InZoneStage,
	int32 InZoneOrder,
	int32 InRegionId)
{
	ZoneId = InZoneId;
	ZoneStage = InZoneStage;
	ZoneOrder = InZoneOrder;
	if (InRegionId != INDEX_NONE)
	{
		CorruptionRegionId = InRegionId;
		if (RegionsToRevive.IsEmpty())
		{
			RegionsToRevive.Add(InRegionId);
		}
	}
}

int32 AGP_EnemySpawnVolume::GetCorruptionRegionId() const
{
	if (CorruptionRegionId != INDEX_NONE)
	{
		return CorruptionRegionId;
	}

	if (!RegionsToRevive.IsEmpty())
	{
		return RegionsToRevive[0];
	}

	return ZoneOrder;
}

void AGP_EnemySpawnVolume::ActivateMarkers()
{
	for (AGP_EnemySpawnMarker* Marker : Markers)
	{
		if (IsValid(Marker))
		{
			Marker->Activate(this);
		}
	}
}

void AGP_EnemySpawnVolume::NotifyMarkerTriggered(AGP_EnemySpawnMarker* Marker)
{
	OnMarkerTriggered.Broadcast(this, Marker);
}

void AGP_EnemySpawnVolume::HandleBoxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bUnlocked || !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	const APawn* PlayerPawn = Cast<APawn>(OtherActor);
	APlayerState* PlayerState = PlayerPawn ? PlayerPawn->GetPlayerState() : nullptr;
	if (PlayerState)
	{
		OnPlayerEnteredZoneDetailed.Broadcast(this, PlayerState);
	}

	if (!bEntered)
	{
		bEntered = true;
		OnPlayerEnteredZone.Broadcast(this);
	}
}

void AGP_EnemySpawnVolume::HandleBoxEndOverlap(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	const APawn* PlayerPawn = Cast<APawn>(OtherActor);
	APlayerState* PlayerState = PlayerPawn ? PlayerPawn->GetPlayerState() : nullptr;
	if (PlayerState)
	{
		OnPlayerExitedZoneDetailed.Broadcast(this, PlayerState);
	}
}

bool AGP_EnemySpawnVolume::TryGetReachableSpawnPoint(
	const FVector& DesiredAnchor,
	float ScatterRadius,
	const FVector& ProjectionExtent,
	FVector& OutSpawnLocation) const
{
	FGPGroundPlacementSettings Settings =
		GPEnemySpawnVolume::AuthoredPointSettings;
	Settings.ProjectionExtent = ProjectionExtent;
	Settings.MaxHeightAboveAnchor =
		FMath::Max(0.0f, MaxSpawnHeightAboveGroundAnchor);

	FVector GroundAnchor;
	if (!GPGroundPlacement::TryProjectGroundAnchor(
			GetWorld(),
			DesiredAnchor,
			Settings,
			GroundAnchor)
		|| !IsPointInsideSpawnBox2D(GroundAnchor))
	{
		return false;
	}

	const float ReachableRadius = FMath::Max(0.0f, ScatterRadius);
	if (ReachableRadius <= KINDA_SMALL_NUMBER)
	{
		OutSpawnLocation = GroundAnchor;
		return true;
	}

	// Keep the zone-bound predicate outside the common navigation utility.
	// One random query per iteration preserves the authored eight-attempt
	// behavior while allowing out-of-zone candidates to be retried.
	Settings.MaxAttempts = 1;
	for (int32 AttemptIndex = 0;
		AttemptIndex < GPEnemySpawnVolume::SpawnProjectionAttempts;
		++AttemptIndex)
	{
		FVector ReachableLocation;
		if (GPGroundPlacement::TryGetRandomReachableGround(
				GetWorld(),
				GroundAnchor,
				ReachableRadius,
				Settings,
				ReachableLocation)
			&& IsPointInsideSpawnBox2D(ReachableLocation))
		{
			OutSpawnLocation = ReachableLocation;
			return true;
		}
	}

	// The authored anchor is already a valid nav location. Prefer it over
	// widening the query to an unrelated roof nav island.
	OutSpawnLocation = GroundAnchor;
	return true;
}

FVector AGP_EnemySpawnVolume::GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const
{
	bOutProjected = false;

	if (!EnemySpawnPoints.IsEmpty())
	{
		for (int32 AttemptIndex = 0; AttemptIndex < GPEnemySpawnVolume::SpawnProjectionAttempts; ++AttemptIndex)
		{
			const AActor* SpawnPoint = EnemySpawnPoints[FMath::RandRange(0, EnemySpawnPoints.Num() - 1)];
			if (!IsValid(SpawnPoint))
			{
				continue;
			}

			FVector ReachableLocation;
			if (TryGetReachableSpawnPoint(
				SpawnPoint->GetActorLocation(),
				EnemySpawnPointScatterRadius,
				GPEnemySpawnVolume::AuthoredPointSettings.ProjectionExtent,
				ReachableLocation))
			{
				bOutProjected = true;
				return ReachableLocation;
			}
		}

		// Authored points are the trusted ground anchors for this zone. If they
		// are temporarily unavailable, let the GameMode retry instead of
		// falling through to a broad query that can select a roof.
		return SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation();
	}

	const FVector CenterLocation =
		SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation();
	const FVector BoxExtent =
		SpawnBox ? SpawnBox->GetScaledBoxExtent() : FVector::ZeroVector;
	const float ScatterRadius =
		bRandomizeInVolume ? FMath::Max(BoxExtent.X, BoxExtent.Y) : 0.0f;
	FVector ReachableLocation;
	if (TryGetReachableSpawnPoint(
		CenterLocation,
		ScatterRadius,
		GPEnemySpawnVolume::AuthoredPointSettings.ProjectionExtent,
		ReachableLocation))
	{
		bOutProjected = true;
		return ReachableLocation;
	}

	bOutProjected = false;
	return CenterLocation;
}

FVector AGP_EnemySpawnVolume::GetBossSpawnPoint(bool& bOutProjected) const
{
	const FVector DesiredLocation =
		IsValid(BossSpawnPoint)
			? BossSpawnPoint->GetActorLocation()
			: (SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation());

	// Most authored boss points are already close to their floor. Resolve them
	// with the same tight query used by regular ground anchors first.
	FGPGroundPlacementSettings AuthoredSettings =
		GPEnemySpawnVolume::AuthoredPointSettings;
	AuthoredSettings.MaxHeightAboveAnchor =
		FMath::Max(0.0f, MaxSpawnHeightAboveGroundAnchor);
	FVector ProjectedLocation;
	bOutProjected = GPGroundPlacement::TryProjectGroundAnchor(
		GetWorld(),
		DesiredLocation,
		AuthoredSettings,
		ProjectedLocation);
	if (bOutProjected && IsPointInsideSpawnBox2D(ProjectedLocation))
	{
		return ProjectedLocation;
	}
	bOutProjected = false;

	// Some Level Instance pivots leave the boss point far above its floor. A
	// broad downward projection is accepted only if a tight, trusted ground
	// anchor in this zone can path to it; isolated roof islands therefore fail.
	FGPGroundPlacementSettings BossSettings =
		GPEnemySpawnVolume::BossPointSettings;
	BossSettings.MaxHeightAboveAnchor =
		FMath::Max(0.0f, MaxSpawnHeightAboveGroundAnchor);
	bOutProjected = GPGroundPlacement::TryProjectGroundAnchor(
		GetWorld(),
		DesiredLocation,
		BossSettings,
		ProjectedLocation);
	if (bOutProjected
		&& IsPointInsideSpawnBox2D(ProjectedLocation)
		&& IsNavLocationReachableFromGroundAnchor(ProjectedLocation))
	{
		return ProjectedLocation;
	}
	bOutProjected = false;

	// Level-instance pivots can leave an authored boss point vertically offset
	// from the landscape. Regular enemy points in this same encounter have
	// already proven that their nearby nav tiles are usable, so prefer the
	// closest one over spawning a boss at an unprojected raw position.
	const AActor* ClosestProjectedPoint = nullptr;
	float ClosestDistanceSquared = TNumericLimits<float>::Max();
	for (const AActor* EnemySpawnPoint : EnemySpawnPoints)
	{
		if (!IsValid(EnemySpawnPoint))
		{
			continue;
		}

		FVector CandidateLocation;
		if (!TryGetReachableSpawnPoint(
			EnemySpawnPoint->GetActorLocation(),
			0.0f,
			GPEnemySpawnVolume::AuthoredPointSettings.ProjectionExtent,
			CandidateLocation))
		{
			continue;
		}

		const float DistanceSquared =
			FVector::DistSquared2D(DesiredLocation, CandidateLocation);
		if (DistanceSquared < ClosestDistanceSquared)
		{
			ClosestDistanceSquared = DistanceSquared;
			ClosestProjectedPoint = EnemySpawnPoint;
			ProjectedLocation = CandidateLocation;
		}
	}

	bOutProjected = IsValid(ClosestProjectedPoint);
	return bOutProjected ? ProjectedLocation : DesiredLocation;
}

bool AGP_EnemySpawnVolume::IsNavLocationReachableFromGroundAnchor(
	const FVector& CandidateLocation) const
{
	auto HasPathFrom = [this, &CandidateLocation](
		const FVector& DesiredGroundAnchor)
	{
		FGPGroundPlacementSettings Settings =
			GPEnemySpawnVolume::AuthoredPointSettings;
		Settings.MaxHeightAboveAnchor =
			FMath::Max(0.0f, MaxSpawnHeightAboveGroundAnchor);
		FVector GroundAnchor;
		if (!GPGroundPlacement::TryProjectGroundAnchor(
				GetWorld(),
				DesiredGroundAnchor,
				Settings,
				GroundAnchor)
			|| !IsPointInsideSpawnBox2D(GroundAnchor))
		{
			return false;
		}

		return GPGroundPlacement::IsReachableGround(
			GetWorld(),
			GroundAnchor,
			CandidateLocation,
			MaxSpawnHeightAboveGroundAnchor);
	};

	for (const AActor* EnemySpawnPoint : EnemySpawnPoints)
	{
		if (IsValid(EnemySpawnPoint)
			&& HasPathFrom(EnemySpawnPoint->GetActorLocation()))
		{
			return true;
		}
	}

	const FVector CenterLocation =
		SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation();
	return HasPathFrom(CenterLocation);
}

FVector AGP_EnemySpawnVolume::GetSpawnPointNearMarker(const AGP_EnemySpawnMarker* Marker, bool& bOutProjected) const
{
	if (!Marker)
	{
		bOutProjected = false;
		return GetActorLocation();
	}

	return GetSpawnPointNearLocation(
		Marker->GetActorLocation(),
		Marker->GetSpawnScatterRadius(),
		bOutProjected);
}

FVector AGP_EnemySpawnVolume::GetSpawnPointNearLocation(
	const FVector& AnchorLocation,
	float ScatterRadius,
	bool& bOutProjected) const
{
	FVector ReachableLocation;
	bOutProjected = TryGetReachableSpawnPoint(
		AnchorLocation,
		ScatterRadius,
		GPEnemySpawnVolume::AuthoredPointSettings.ProjectionExtent,
		ReachableLocation);
	return bOutProjected ? ReachableLocation : AnchorLocation;
}

bool AGP_EnemySpawnVolume::IsPointInsideSpawnBox(const FVector& WorldLocation) const
{
	if (!SpawnBox)
	{
		return false;
	}

	const FVector Local =
		SpawnBox->GetComponentTransform().InverseTransformPositionNoScale(
			WorldLocation);
	const FVector Extent = SpawnBox->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y
		&& FMath::Abs(Local.Z) <= Extent.Z;
}

bool AGP_EnemySpawnVolume::IsPointInsideSpawnBox2D(const FVector& WorldLocation) const
{
	if (!SpawnBox)
	{
		return false;
	}

	const FVector Local =
		SpawnBox->GetComponentTransform().InverseTransformPositionNoScale(
			WorldLocation);
	const FVector Extent = SpawnBox->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y;
}

bool AGP_EnemySpawnVolume::IsSpawnGroundLocationValid(
	const FVector& GroundAnchor,
	const FVector& CandidateGroundLocation) const
{
	return !GroundAnchor.ContainsNaN()
		&& GPGroundPlacement::IsGroundRiseWithinLimit(
			GroundAnchor,
			CandidateGroundLocation,
			MaxSpawnHeightAboveGroundAnchor)
		&& IsPointInsideSpawnBox2D(CandidateGroundLocation);
}

bool AGP_EnemySpawnVolume::IsFinalSpawnGroundLocationValid(
	const FVector& RequestedGroundLocation,
	const FVector& ActualGroundLocation) const
{
	return !RequestedGroundLocation.ContainsNaN()
		&& GPGroundPlacement::IsGroundRiseWithinLimit(
			RequestedGroundLocation,
			ActualGroundLocation,
			MaxFinalSpawnHeightAboveNavmesh)
		&& IsPointInsideSpawnBox2D(ActualGroundLocation);
}

void AGP_EnemySpawnVolume::CollectTaggedSpawnPoints()
{
	auto CollectPointsWithTag = [this](FName Tag, TArray<TObjectPtr<AActor>>& OutPoints)
	{
		if (Tag.IsNone())
		{
			return;
		}

		TArray<AActor*> TaggedActors;
		UGameplayStatics::GetAllActorsWithTag(this, Tag, TaggedActors);
		for (AActor* Actor : TaggedActors)
		{
			if (IsValid(Actor)
				&& Actor->GetLevel() == GetLevel()
				&& IsPointInsideSpawnBox(Actor->GetActorLocation()))
			{
				OutPoints.Add(Actor);
			}
		}
	};

	CollectPointsWithTag(EnemySpawnPointTag, EnemySpawnPoints);

	TArray<TObjectPtr<AActor>> BossPoints;
	CollectPointsWithTag(BossSpawnPointTag, BossPoints);
	if (!BossPoints.IsEmpty())
	{
		BossSpawnPoint = BossPoints[0];
	}

	UE_LOG(LogTemp, Log,
		TEXT("[GP_EnemySpawnVolume] Zone '%s' collected %d enemy spawn point(s), boss point=%s."),
		*GetName(),
		EnemySpawnPoints.Num(),
		IsValid(BossSpawnPoint) ? *BossSpawnPoint->GetName() : TEXT("None"));
}
