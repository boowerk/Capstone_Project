#include "Game/GP_LobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Game/GP_LobbyPlayerState.h"
#include "GameFramework/PlayerController.h"
#include "UI/GP_LobbyWidget.h"

AGP_LobbyGameMode::AGP_LobbyGameMode()
{
	PlayerStateClass = AGP_LobbyPlayerState::StaticClass();
}

void AGP_LobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !LobbyWidgetClass)
	{
		return;
	}

	LobbyWidget = CreateWidget<UGP_LobbyWidget>(PC, LobbyWidgetClass);
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		PC->SetInputMode(FInputModeUIOnly());
		PC->SetShowMouseCursor(true);
	}
}

void AGP_LobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (AGP_LobbyPlayerState* LPS = NewPlayer->GetPlayerState<AGP_LobbyPlayerState>())
	{
		LPS->OnReadyChanged.AddDynamic(this, &AGP_LobbyGameMode::OnPlayerReadyChanged);
	}

	OnPlayerCountChanged(GetNumPlayers());

	if (IsValid(LobbyWidget))
	{
		LobbyWidget->NotifyPlayerListChanged();
	}
}

void AGP_LobbyGameMode::Logout(AController* Exiting)
{
	Super::Logout(Exiting);
	OnPlayerCountChanged(GetNumPlayers() - 1);
}

void AGP_LobbyGameMode::OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bIsReady)
{
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
