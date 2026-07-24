#include "Game/GP_LobbyGameMode.h"

#include "Blueprint/UserWidget.h"
#include "Game/GP_LobbyPlayerController.h"
#include "Game/GP_LobbyPlayerState.h"
#include "Game/GP_RunSeed.h"
#include "Game/GP_ThreePlayerGameSession.h"
#include "Game/GP_ThreePlayerGameSession.h"
#include "Game/GP_ThreePlayerGameSession.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/GP_LobbyWidget.h"

namespace
{
	bool IsLobbySmokeEnabled()
	{
#if !UE_BUILD_SHIPPING
		return FParse::Param(FCommandLine::Get(), TEXT("LobbySmokeAutoReady"));
#else
		return false;
#endif
	}
}

AGP_LobbyGameMode::AGP_LobbyGameMode()
{
	PlayerControllerClass = AGP_LobbyPlayerController::StaticClass();
	PlayerStateClass = AGP_LobbyPlayerState::StaticClass();
	// Both lobby admission and gameplay travel share the same fixed
	// three-player server contract.
	GameSessionClass = AGP_ThreePlayerGameSession::StaticClass();

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

void AGP_LobbyGameMode::BroadcastLoadingCancelled()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AGP_LobbyPlayerController* LPC = Cast<AGP_LobbyPlayerController>(It->Get()))
		{
			LPC->ClientHideLoading();
		}
	}
}

void AGP_LobbyGameMode::OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bIsReady)
{
	if (IsLobbySmokeEnabled())
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[LobbySmoke] server-ready player=%s ready=%s players=%d expected=%d"),
			*GetNameSafe(PlayerState),
			bIsReady ? TEXT("true") : TEXT("false"),
			GetNumPlayers(),
			ExpectedPlayerCount);
	}
	BroadcastRefreshPlayerList();
	CheckAllReady();
}

void AGP_LobbyGameMode::CheckAllReady()
{
	if (bTravelInitiated)
	{
		return;
	}

	const int32 PlayerCount = GetNumPlayers();
	if (!HasExactPartySize(PlayerCount, ExpectedPlayerCount))
	{
		if (IsLobbySmokeEnabled())
		{
			UE_LOG(
				LogTemp,
				Display,
				TEXT("[LobbySmoke] ready-gate=player-count players=%d expected=%d"),
				PlayerCount,
				ExpectedPlayerCount);
		}
		return;
	}

	int32 ReadyPlayerCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		const AGP_LobbyPlayerState* LPS = It->Get()->GetPlayerState<AGP_LobbyPlayerState>();
		if (!LPS || !LPS->IsReady())
		{
			if (IsLobbySmokeEnabled())
			{
				UE_LOG(
					LogTemp,
					Display,
					TEXT("[LobbySmoke] ready-gate=readiness ready=%d players=%d expected=%d"),
					ReadyPlayerCount,
					PlayerCount,
					ExpectedPlayerCount);
			}
			return;
		}
		++ReadyPlayerCount;
	}

	if (IsLobbySmokeEnabled())
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[LobbySmoke] all-ready ready=%d players=%d expected=%d destination=%s"),
			ReadyPlayerCount,
			PlayerCount,
			ExpectedPlayerCount,
			*GameMapName);
	}
	bTravelInitiated = true;
	OnAllPlayersReady();
	BroadcastLoading();
	TravelToGame();
}

bool AGP_LobbyGameMode::HasExactPartySize(int32 ConnectedPlayerCount, int32 RequiredPlayerCount)
{
	// Do not treat an over-capacity lobby as ready even if every player has
	// toggled Ready; only the authored party size may begin the run.
	return ConnectedPlayerCount == RequiredPlayerCount;
}

bool AGP_LobbyGameMode::CanForceStart(const APlayerController* RequestingController) const
{
	const bool bExplicitlyEnabled =
		FParse::Param(FCommandLine::Get(), TEXT("AllowLobbyForceStart"));
	return IsForceStartRequestAllowed(
		bExplicitlyEnabled,
		IsValid(RequestingController) && RequestingController->HasAuthority(),
		IsValid(RequestingController) && RequestingController->IsLocalController(),
		GetNetMode() == NM_DedicatedServer);
}

bool AGP_LobbyGameMode::TryForceStartGame(APlayerController* RequestingController)
{
	if (!CanForceStart(RequestingController))
	{
		// Remote clients and production builds must never bypass the exact
		// three-player Ready contract through the debug RPC.
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("[Lobby] Rejected force-start request from %s."),
			*GetNameSafe(RequestingController));
		return false;
	}

	if (bTravelInitiated)
	{
		return false;
	}
	bTravelInitiated = true;
	// Explicit local debug sessions may skip the count and Ready gates.
	OnAllPlayersReady();
	BroadcastLoading();
	TravelToGame();
	return true;
}

bool AGP_LobbyGameMode::IsForceStartRequestAllowed(
	bool bExplicitlyEnabled,
	bool bHasAuthority,
	bool bIsLocalController,
	bool bIsDedicatedServer)
{
#if UE_BUILD_SHIPPING
	return false;
#else
	return bExplicitlyEnabled
		&& bHasAuthority
		&& (bIsLocalController || bIsDedicatedServer);
#endif
}

void AGP_LobbyGameMode::TravelToGame()
{
	if (!HasAuthority() || bTravelStarted)
	{
		return;
	}

	bTravelStarted = true;

	// Restore game input for all players before travelling.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->SetInputMode(FInputModeGameOnly());
			PC->SetShowMouseCursor(false);
		}
	}

	const int32 RunSeed = GPRunSeed::Generate();
	const FString URL = GPRunSeed::BuildTravelURL(GameMapName, RunSeed);
	UE_LOG(LogTemp, Log, TEXT("[GP_LobbyGameMode] Starting run with seed %d."), RunSeed);

	if (!GetWorld()->ServerTravel(URL))
	{
		bTravelStarted = false;
		bTravelInitiated = false;
		BroadcastLoadingCancelled();
		UE_LOG(LogTemp, Error, TEXT("[GP_LobbyGameMode] ServerTravel failed for '%s'."), *URL);
	}
}
