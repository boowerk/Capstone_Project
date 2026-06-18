#include "Game/GP_RunPortal.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"

AGP_RunPortal::AGP_RunPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
	SetRootComponent(Trigger);
	Trigger->SetSphereRadius(150.0f);
	Trigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Trigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	Trigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Trigger->SetGenerateOverlapEvents(true);
	Trigger->ShapeColor = FColor(120, 120, 255);
}

void AGP_RunPortal::BeginPlay()
{
	Super::BeginPlay();

	// Only the server decides teleport/progression.
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGP_RunPortal::HandleOverlapBegin);
	}
}

void AGP_RunPortal::HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AGP_PlayerCharacter* Player = Cast<AGP_PlayerCharacter>(OtherActor);
	if (!Player || PlayersEntered.Contains(Player))
	{
		return;
	}

	PlayersEntered.Add(Player);
	Player->TeleportTo(TargetLocation, Player->GetActorRotation());
	OnPlayerPassedThrough(Player);

	// Destroy when every player in the level has passed through.
	TArray<AActor*> AllPlayers;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_PlayerCharacter::StaticClass(), AllPlayers);

	const bool bAllPassed = AllPlayers.Num() > 0 &&
		PlayersEntered.Num() >= AllPlayers.Num();

	if (bAllPassed)
	{
		OnPortalActivated();
		// Delay destroy by one frame so the TeleportTo replication reaches clients
		// before the actor is torn off.
		SetLifeSpan(0.1f);
	}
}
