#include "Game/GP_EnemySpawnVolume.h"

#include "Components/BoxComponent.h"
#include "NavigationSystem.h"

AGP_EnemySpawnVolume::AGP_EnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	// The box only marks the spawn area; it should not block movement or generate overlaps.
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpawnBox->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnBox->ShapeColor = FColor(80, 200, 120);
}

FVector AGP_EnemySpawnVolume::GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const
{
	bOutProjected = false;

	const FVector Center = SpawnBox->GetComponentLocation();
	FVector DesiredLocation = Center;

	if (bRandomizeInVolume)
	{
		// Pick a random point inside the (possibly rotated/scaled) box, matching the placed combat area.
		const FVector Extent = SpawnBox->GetScaledBoxExtent();
		const FVector LocalOffset(
			FMath::FRandRange(-Extent.X, Extent.X),
			FMath::FRandRange(-Extent.Y, Extent.Y),
			0.0f);
		DesiredLocation = SpawnBox->GetComponentTransform().TransformPositionNoScale(LocalOffset);
	}

	// Project onto navmesh so spawned enemies can immediately path with their shared BT (same approach as
	// UGP_BossSummonAdds). The query extent is generous to recover from points above/below the floor.
	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, FVector(600.0f, 600.0f, 1000.0f)))
		{
			bOutProjected = true;
			return ProjectedLocation.Location;
		}
	}

	return DesiredLocation;
}
