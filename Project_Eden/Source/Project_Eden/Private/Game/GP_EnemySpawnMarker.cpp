#include "Game/GP_EnemySpawnMarker.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"

AGP_EnemySpawnMarker::AGP_EnemySpawnMarker()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetSphereRadius(400.0f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor(230, 160, 60);
}

void AGP_EnemySpawnMarker::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGP_EnemySpawnMarker::HandleOverlapBegin);
	}
}

void AGP_EnemySpawnMarker::Activate(AGP_EnemySpawnVolume* OwningZone)
{
	OwnerZone = OwningZone;
	bActive = true;
}

void AGP_EnemySpawnMarker::HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bTriggered || !bActive || !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	bTriggered = true;

	if (IsValid(OwnerZone))
	{
		OwnerZone->NotifyMarkerTriggered(this);
	}
}
