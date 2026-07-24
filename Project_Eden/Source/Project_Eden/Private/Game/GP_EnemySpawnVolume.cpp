#include "Game/GP_EnemySpawnVolume.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

namespace GPEnemySpawnVolume
{
	constexpr int32 SpawnProjectionAttempts = 8;
	constexpr float MinProjectionHorizontalExtent = 600.0f;
	constexpr float MinProjectionVerticalExtent = 1000.0f;
	constexpr float ProjectionVerticalPadding = 200.0f;
	const FVector AuthoredPointProjectionExtent(200.0f, 200.0f, 300.0f);
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
			const FVector Local = SpawnBox->GetComponentTransform().InverseTransformPosition(Marker->GetActorLocation());
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

FVector AGP_EnemySpawnVolume::ProjectToNavmesh(const FVector& DesiredLocation, bool& bOutProjected) const
{
	return ProjectToNavmesh(DesiredLocation, GetSpawnProjectionExtent(), bOutProjected);
}

FVector AGP_EnemySpawnVolume::ProjectToNavmesh(const FVector& DesiredLocation, const FVector& QueryExtent, bool& bOutProjected) const
{
	bOutProjected = false;

	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, QueryExtent))
		{
			bOutProjected = true;
			return ProjectedLocation.Location;
		}
	}

	return DesiredLocation;
}

FVector AGP_EnemySpawnVolume::GetSpawnProjectionExtent() const
{
	FVector QueryExtent(
		GPEnemySpawnVolume::MinProjectionHorizontalExtent,
		GPEnemySpawnVolume::MinProjectionHorizontalExtent,
		GPEnemySpawnVolume::MinProjectionVerticalExtent);

	if (SpawnBox)
	{
		const FVector BoxExtent = SpawnBox->GetScaledBoxExtent();
		QueryExtent.Z = FMath::Max(QueryExtent.Z, BoxExtent.Z + GPEnemySpawnVolume::ProjectionVerticalPadding);
	}

	return QueryExtent;
}

FVector AGP_EnemySpawnVolume::GetRandomPointInSpawnBox() const
{
	if (!SpawnBox)
	{
		return GetActorLocation();
	}

	const FVector Extent = SpawnBox->GetScaledBoxExtent();
	const FVector LocalOffset(
		FMath::FRandRange(-Extent.X, Extent.X),
		FMath::FRandRange(-Extent.Y, Extent.Y),
		0.0f);

	return SpawnBox->GetComponentTransform().TransformPositionNoScale(LocalOffset);
}

FVector AGP_EnemySpawnVolume::GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const
{
	if (!EnemySpawnPoints.IsEmpty())
	{
		for (int32 AttemptIndex = 0; AttemptIndex < GPEnemySpawnVolume::SpawnProjectionAttempts; ++AttemptIndex)
		{
			const AActor* SpawnPoint = EnemySpawnPoints[FMath::RandRange(0, EnemySpawnPoints.Num() - 1)];
			if (!IsValid(SpawnPoint))
			{
				continue;
			}

			const FVector DesiredLocation =
				GetPointWithScatter(SpawnPoint->GetActorLocation(), EnemySpawnPointScatterRadius);
			const FVector ProjectedLocation = ProjectToNavmesh(
				DesiredLocation,
				GPEnemySpawnVolume::AuthoredPointProjectionExtent,
				bOutProjected);
			if (bOutProjected)
			{
				return ProjectedLocation;
			}
		}
	}

	if (bRandomizeInVolume)
	{
		for (int32 AttemptIndex = 0; AttemptIndex < GPEnemySpawnVolume::SpawnProjectionAttempts; ++AttemptIndex)
		{
			const FVector DesiredLocation = GetRandomPointInSpawnBox();
			const FVector ProjectedLocation = ProjectToNavmesh(DesiredLocation, bOutProjected);
			if (bOutProjected)
			{
				return ProjectedLocation;
			}
		}
	}

	const FVector CenterLocation = SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation();
	FVector ProjectedLocation = ProjectToNavmesh(CenterLocation, bOutProjected);
	if (bOutProjected)
	{
		return ProjectedLocation;
	}

	if (!bRandomizeInVolume)
	{
		for (int32 AttemptIndex = 0; AttemptIndex < GPEnemySpawnVolume::SpawnProjectionAttempts; ++AttemptIndex)
		{
			const FVector DesiredLocation = GetRandomPointInSpawnBox();
			ProjectedLocation = ProjectToNavmesh(DesiredLocation, bOutProjected);
			if (bOutProjected)
			{
				return ProjectedLocation;
			}
		}
	}

	return CenterLocation;
}

FVector AGP_EnemySpawnVolume::GetBossSpawnPoint(bool& bOutProjected) const
{
	const FVector DesiredLocation =
		IsValid(BossSpawnPoint)
			? BossSpawnPoint->GetActorLocation()
			: (SpawnBox ? SpawnBox->GetComponentLocation() : GetActorLocation());

	return ProjectToNavmesh(
		DesiredLocation,
		GPEnemySpawnVolume::AuthoredPointProjectionExtent,
		bOutProjected);
}

FVector AGP_EnemySpawnVolume::GetSpawnPointNearMarker(const AGP_EnemySpawnMarker* Marker, bool& bOutProjected) const
{
	if (!Marker)
	{
		bOutProjected = false;
		return GetActorLocation();
	}

	const float Scatter = Marker->GetSpawnScatterRadius();
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Dist = Scatter * FMath::Sqrt(FMath::FRand());
	const FVector DesiredLocation = Marker->GetActorLocation()
		+ FVector(Dist * FMath::Cos(Angle), Dist * FMath::Sin(Angle), 0.0f);

	return ProjectToNavmesh(DesiredLocation, bOutProjected);
}

FVector AGP_EnemySpawnVolume::GetPointWithScatter(const FVector& Origin, float ScatterRadius) const
{
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Distance = FMath::Max(0.0f, ScatterRadius) * FMath::Sqrt(FMath::FRand());
	return Origin + FVector(Distance * FMath::Cos(Angle), Distance * FMath::Sin(Angle), 0.0f);
}

bool AGP_EnemySpawnVolume::IsPointInsideSpawnBox(const FVector& WorldLocation) const
{
	if (!SpawnBox)
	{
		return false;
	}

	const FVector Local = SpawnBox->GetComponentTransform().InverseTransformPosition(WorldLocation);
	const FVector Extent = SpawnBox->GetScaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X
		&& FMath::Abs(Local.Y) <= Extent.Y
		&& FMath::Abs(Local.Z) <= Extent.Z;
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
