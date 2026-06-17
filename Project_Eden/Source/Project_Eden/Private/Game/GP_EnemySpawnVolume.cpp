#include "Game/GP_EnemySpawnVolume.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "NavigationSystem.h"

AGP_EnemySpawnVolume::AGP_EnemySpawnVolume()
{
	PrimaryActorTick.bCanEverTick = false;

	SpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnBox"));
	SetRootComponent(SpawnBox);
	SpawnBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SpawnBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	SpawnBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SpawnBox->SetBoxExtent(FVector(500.0f, 500.0f, 200.0f));
	SpawnBox->ShapeColor = FColor(80, 200, 120);
}

void AGP_EnemySpawnVolume::BeginPlay()
{
	Super::BeginPlay();
	SpawnBox->OnComponentBeginOverlap.AddDynamic(this, &AGP_EnemySpawnVolume::HandleOverlapBegin);
}

void AGP_EnemySpawnVolume::Unlock()
{
	bUnlocked = true;
}

void AGP_EnemySpawnVolume::HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTriggered || !bUnlocked)
	{
		return;
	}

	if (!Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	bTriggered = true;
	OnPlayerEnteredZone.Broadcast(this);
}

FVector AGP_EnemySpawnVolume::GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const
{
	bOutProjected = false;

	const FVector Center = SpawnBox->GetComponentLocation();
	FVector DesiredLocation = Center;

	if (bRandomizeInVolume)
	{
		const FVector Extent = SpawnBox->GetScaledBoxExtent();
		const FVector LocalOffset(
			FMath::FRandRange(-Extent.X, Extent.X),
			FMath::FRandRange(-Extent.Y, Extent.Y),
			0.0f);
		DesiredLocation = SpawnBox->GetComponentTransform().TransformPositionNoScale(LocalOffset);
	}

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
