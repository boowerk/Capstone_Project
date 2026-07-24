#include "Game/GP_RunPortal.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "Game/GP_GameMode.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Player/GP_PlayerController.h"
#include "TimerManager.h"

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

void AGP_RunPortal::SetDestination(
	AGP_EnemySpawnVolume* InTargetZone,
	const FVector& InTargetLocation)
{
	TargetZone = InTargetZone;
	TargetLocation = InTargetLocation;
	bDestinationConfigured = IsValid(InTargetZone);
}

void AGP_RunPortal::SetMiddleDestinationSelection(bool bEnabled)
{
	bMiddleDestinationSelection = bEnabled;
	if (bEnabled)
	{
		bDestinationConfigured = true;
	}
}

bool AGP_RunPortal::IsActivePartyComplete(
	int32 ActivePlayerCount,
	int32 EnteredActivePlayerCount)
{
	return ActivePlayerCount > 0
		&& EnteredActivePlayerCount >= ActivePlayerCount;
}

void AGP_RunPortal::BeginPlay()
{
	Super::BeginPlay();

	// Only the server decides teleport/progression.
	if (HasAuthority())
	{
		Trigger->OnComponentBeginOverlap.AddDynamic(this, &AGP_RunPortal::HandleOverlapBegin);
		if (UWorld* World = GetWorld())
		{
			// Component registration can discover an overlap before BeginPlay
			// binds the delegate. Re-scan after deferred spawning is complete.
			World->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					HandleExistingOverlaps();
				}));
		}
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
	TryHandlePlayer(Cast<AGP_PlayerCharacter>(OtherActor));
}

void AGP_RunPortal::HandleExistingOverlaps()
{
	if (!HasAuthority() || !IsValid(Trigger))
	{
		return;
	}

	TArray<AActor*> OverlappingPlayers;
	Trigger->GetOverlappingActors(
		OverlappingPlayers,
		AGP_PlayerCharacter::StaticClass());
	for (AActor* OverlappingActor : OverlappingPlayers)
	{
		TryHandlePlayer(Cast<AGP_PlayerCharacter>(OverlappingActor));
	}
}

void AGP_RunPortal::TryHandlePlayer(AGP_PlayerCharacter* Player)
{
	if (!HasAuthority() || !IsValid(Player))
	{
		return;
	}

	if (!bDestinationConfigured)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Portal] ignored overlap by '%s': destination was not configured before play."),
			*GetNameSafe(Player));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Portal] overlap by %s, target=%s"),
		*GetNameSafe(Player), *TargetLocation.ToString());

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

	APlayerState* PlayerState = Player->GetPlayerState();
	if (!IsValid(PlayerState) || PlayersEntered.Contains(PlayerState))
	{
		return;
	}

	// Lift the target above the navmesh ground point by the capsule half-height so the
	// player doesn't encroach the floor (which makes a collision-checked teleport fail).
	const float HalfHeight = Player->GetSimpleCollisionHalfHeight();
	const FVector SafeTarget = TargetLocation + FVector(0.0f, 0.0f, HalfHeight + 10.0f);
	if (!Player->TeleportTo(
		SafeTarget,
		Player->GetActorRotation(),
		/*bIsATest=*/false,
		/*bNoCheck=*/true))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[Portal] rejected arrival for '%s': teleport to '%s' failed."),
			*GetNameSafe(Player),
			*SafeTarget.ToCompactString());
		return;
	}

	PlayersEntered.Add(PlayerState);

	if (AGP_GameMode* GameMode =
		GetWorld() ? GetWorld()->GetAuthGameMode<AGP_GameMode>() : nullptr)
	{
		GameMode->NotifyPlayerArrivedViaPortal(Player, TargetZone);
	}

	OnPlayerPassedThrough(Player);

	if (HaveAllActivePlayersEntered())
	{
		OnPortalActivated();
		// Delay destroy by one frame so the TeleportTo replication reaches clients
		// before the actor is torn off.
		SetLifeSpan(0.1f);
	}
}

bool AGP_RunPortal::HaveAllActivePlayersEntered() const
{
	const UWorld* World = GetWorld();
	const AGameStateBase* GameState =
		World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return false;
	}

	int32 ActivePlayerCount = 0;
	int32 EnteredActivePlayerCount = 0;
	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!IsValid(PlayerState) || PlayerState->IsOnlyASpectator())
		{
			continue;
		}

		++ActivePlayerCount;
		if (PlayersEntered.Contains(PlayerState))
		{
			++EnteredActivePlayerCount;
		}
	}
	return IsActivePartyComplete(
		ActivePlayerCount,
		EnteredActivePlayerCount);
}
