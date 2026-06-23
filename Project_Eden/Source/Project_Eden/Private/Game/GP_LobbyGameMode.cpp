#include "Game/GP_LobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Game/GP_LobbyPlayerController.h"
#include "Game/GP_LobbyPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "UI/GP_LobbyWidget.h"

AGP_LobbyGameMode::AGP_LobbyGameMode()
{
	PlayerControllerClass = AGP_LobbyPlayerController::StaticClass();
	PlayerStateClass = AGP_LobbyPlayerState::StaticClass();

	// Keep clients connected through the map load. Non-seamless ServerTravel
	// disconnects every client while the server blocks on the map load; a slow
	// load (24s+ observed) then times the clients out and bounces them to the
	// menu. Seamless travel streams the transition without dropping connections.
	bUseSeamlessTravel = true;
}

void AGP_LobbyGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void AGP_LobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AGP_LobbyPlayerState* LPS = NewPlayer->GetPlayerState<AGP_LobbyPlayerState>())
	{
		LPS->OnReadyChanged.AddDynamic(this, &AGP_LobbyGameMode::OnPlayerReadyChanged);
	}

	OnPlayerCountChanged(GetNumPlayers());
	BroadcastRefreshPlayerList();
}

void AGP_LobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	OnPlayerCountChanged(GetNumPlayers() - 1);
	BroadcastRefreshPlayerList();
}

void AGP_LobbyGameMode::BroadcastRefreshPlayerList()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AGP_LobbyPlayerController* LPC = Cast<AGP_LobbyPlayerController>(It->Get()))
		{
			LPC->ClientRefreshPlayerList();
		}
	}
}

void AGP_LobbyGameMode::BroadcastLoading()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AGP_LobbyPlayerController* LPC = Cast<AGP_LobbyPlayerController>(It->Get()))
		{
			LPC->ClientShowLoading();
		}
	}
}

void AGP_LobbyGameMode::OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bIsReady)
{
	BroadcastRefreshPlayerList();
	CheckAllReady();
}

void AGP_LobbyGameMode::CheckAllReady()
{
	const int32 PlayerCount = GetNumPlayers();
	if (PlayerCount < ExpectedPlayerCount)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AGP_LobbyPlayerState* LPS = It->Get()->GetPlayerState<AGP_LobbyPlayerState>();
		if (!LPS || !LPS->IsReady())
		{
			return;
		}
	}

	OnAllPlayersReady();
	BroadcastLoading();
	TravelToGame();
}

void AGP_LobbyGameMode::ForceStartGame()
{
	// Skip the player-count and ready gates entirely so a lone player can start.
	OnAllPlayersReady();
	BroadcastLoading();
	TravelToGame();
}

void AGP_LobbyGameMode::TravelToGame()
{
	// Restore game input for all players before travelling.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
	}

	const FString URL = FString::Printf(TEXT("/Game/Maps/%s?listen"), *GameMapName);
	GetWorld()->ServerTravel(URL);
}
