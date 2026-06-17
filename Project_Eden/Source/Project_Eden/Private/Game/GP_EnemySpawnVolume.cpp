#include "Game/GP_EnemySpawnVolume.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Game/GP_EnemySpawnMarker.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"

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

	SpawnBox->OnComponentBeginOverlap.AddDynamic(this, &AGP_EnemySpawnVolume::HandleBoxOverlap);

	// Collect AGP_EnemySpawnMarker actors whose centers fall inside this box.
	TArray<AActor*> AllMarkers;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemySpawnMarker::StaticClass(), AllMarkers);
	for (AActor* Actor : AllMarkers)
	{
		if (AGP_EnemySpawnMarker* Marker = Cast<AGP_EnemySpawnMarker>(Actor))
		{
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
}

void AGP_EnemySpawnVolume::Unlock()
{
	bUnlocked = true;
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
	if (bEntered || !bUnlocked || !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	bEntered = true;
	OnPlayerEnteredZone.Broadcast(this);
}

FVector AGP_EnemySpawnVolume::ProjectToNavmesh(const FVector& DesiredLocation, bool& bOutProjected) const
{
	bOutProjected = false;

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

FVector AGP_EnemySpawnVolume::GetSpawnPoint(bool bRandomizeInVolume, bool& bOutProjected) const
{
	FVector DesiredLocation = SpawnBox->GetComponentLocation();

	if (bRandomizeInVolume)
	{
		const FVector Extent = SpawnBox->GetScaledBoxExtent();
		const FVector LocalOffset(
			FMath::FRandRange(-Extent.X, Extent.X),
			FMath::FRandRange(-Extent.Y, Extent.Y),
			0.0f);
		DesiredLocation = SpawnBox->GetComponentTransform().TransformPositionNoScale(LocalOffset);
	}

	return ProjectToNavmesh(DesiredLocation, bOutProjected);
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
