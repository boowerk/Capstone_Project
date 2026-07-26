#include "Game/GP_EnemySpawnMarker.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "TimerManager.h"

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

	// Client copies exist only so their containing village can render locally.
	// Non-replicated actors still report ROLE_Authority on clients.
	if (GetNetMode() == NM_Client)
	{
		Trigger->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		return;
	}

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGP_EnemySpawnMarker::HandleOverlapBegin);
}

void AGP_EnemySpawnMarker::Activate(AGP_EnemySpawnVolume* OwningZone)
{
	OwnerZone = OwningZone;
	bActive = IsValid(OwningZone);
	if (!bActive || bTriggered || GetNetMode() == NM_Client)
	{
		return;
	}

	// A player may already be inside this always-querying sphere when the zone
	// arms it. Defer the scan so legacy StartZone can finish broadcasting its
	// start event before an empty marker is allowed to complete the zone.
	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			EvaluateExistingOverlaps();
		}));
}

void AGP_EnemySpawnMarker::HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TryTriggerForActor(OtherActor);
}

bool AGP_EnemySpawnMarker::TryTriggerForActor(AActor* OtherActor)
{
	if (bTriggered
		|| !bActive
		|| !IsValid(OwnerZone)
		|| !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return false;
	}

	bTriggered = true;
	OwnerZone->NotifyMarkerTriggered(this);
	return true;
}

void AGP_EnemySpawnMarker::EvaluateExistingOverlaps()
{
	if (bTriggered || !bActive || !IsValid(OwnerZone) || !IsValid(Trigger))
	{
		return;
	}

	Trigger->UpdateOverlaps();
	TArray<AActor*> OverlappingPlayers;
	Trigger->GetOverlappingActors(
		OverlappingPlayers,
		AGP_PlayerCharacter::StaticClass());
	for (AActor* OverlappingPlayer : OverlappingPlayers)
	{
		if (TryTriggerForActor(OverlappingPlayer))
		{
			return;
		}
	}
}
