#include "Game/GP_RunPortal.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/GP_GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Player/GP_PlayerController.h"

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

void AGP_RunPortal::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_RunPortal, bMiddleDestinationSelection);
}

void AGP_RunPortal::HandleOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	UE_LOG(LogTemp, Warning, TEXT("[Portal] overlap by %s, target=%s"),
		*GetNameSafe(OtherActor), *TargetLocation.ToString());

	AGP_PlayerCharacter* Player = Cast<AGP_PlayerCharacter>(OtherActor);
	if (!Player || PlayersEntered.Contains(Player))
	{
		return;
	}

	if (bMiddleDestinationSelection)
	{
		if (AGP_PlayerController* PlayerController =
			Cast<AGP_PlayerController>(Player->GetController()))
		{
			if (AGP_GameMode* GameMode =
				GetWorld() ? GetWorld()->GetAuthGameMode<AGP_GameMode>() : nullptr)
			{
				GameMode->OpenMiddleTravelSelection(PlayerController);
				OnMiddleDestinationSelectionOpened(Player);
			}
		}
		return;
	}

	PlayersEntered.Add(Player);

	// Lift the target above the navmesh ground point by the capsule half-height so the
	// player doesn't encroach the floor (which makes a collision-checked teleport fail).
	const float HalfHeight = Player->GetSimpleCollisionHalfHeight();
	const FVector SafeTarget = TargetLocation + FVector(0.0f, 0.0f, HalfHeight + 10.0f);
	Player->SetActorLocation(SafeTarget, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);

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
