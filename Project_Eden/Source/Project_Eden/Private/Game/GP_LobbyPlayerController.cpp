#include "Game/GP_LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Game/GP_GameInstance.h"
#include "Game/GP_LobbyGameMode.h"
#include "Game/GP_LobbyPlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UI/GP_LobbyWidget.h"

void AGP_LobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

#if !UE_BUILD_SHIPPING
	BeginLobbySmokeAutoReady();
#endif

	if (!IsLocalController() || !LobbyWidgetClass)
	{
		return;
	}

	// Only show the lobby UI while on the lobby map. After ServerTravel to a
	// game map this controller may still be used; creating the widget there
	// would force UIOnly input and block gameplay.
	const FString MapName = GetWorld() ? GetWorld()->GetMapName() : FString();
	if (!MapName.Contains(TEXT("LobbyMap")))
	{
		return;
	}

	LobbyWidget = CreateWidget<UGP_LobbyWidget>(this, LobbyWidgetClass);
	if (LobbyWidget)
	{
		LobbyWidget->AddToViewport();
		SetInputMode(FInputModeUIOnly());
		SetShowMouseCursor(true);
	}
}

#if !UE_BUILD_SHIPPING
void AGP_LobbyPlayerController::BeginLobbySmokeAutoReady()
{
	const UWorld* World = GetWorld();
	// Seamless travel 뒤 목적지 컨트롤러에서 테스트 준비 RPC가 다시 실행되지 않도록 로비에서만 허용한다.
	if (!IsLocalController()
		|| !IsValid(World)
		|| !World->GetMapName().Contains(TEXT("LobbyMap"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("LobbySmokeAutoReady")))
	{
		return;
	}

	LobbySmokeReadyAttempts = 0;
	// Delay the first attempt until the server has completed PostLogin and
	// bound the PlayerState Ready delegate.
	GetWorldTimerManager().SetTimer(
		LobbySmokeReadyTimerHandle,
		this,
		&ThisClass::TryLobbySmokeAutoReady,
		0.75f,
		false);
}

void AGP_LobbyPlayerController::TryLobbySmokeAutoReady()
{
	++LobbySmokeReadyAttempts;
	if (AGP_LobbyPlayerState* LobbyPlayerState = GetPlayerState<AGP_LobbyPlayerState>())
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[LobbySmoke] client-ready controller=%s attempt=%d"),
			*GetNameSafe(this),
			LobbySmokeReadyAttempts);
		LobbyPlayerState->ServerSetReady(true);
		return;
	}

	constexpr int32 MaxReadyAttempts = 20;
	if (LobbySmokeReadyAttempts >= MaxReadyAttempts)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[LobbySmoke] client-ready-timeout controller=%s attempts=%d"),
			*GetNameSafe(this),
			LobbySmokeReadyAttempts);
		return;
	}

	GetWorldTimerManager().SetTimer(
		LobbySmokeReadyTimerHandle,
		this,
		&ThisClass::TryLobbySmokeAutoReady,
		0.25f,
		false);
}
#endif

void AGP_LobbyPlayerController::ClientRefreshPlayerList_Implementation()
{
	if (IsValid(LobbyWidget))
	{
		LobbyWidget->NotifyPlayerListChanged();
	}
}

void AGP_LobbyPlayerController::ClientShowLoading_Implementation()
{
	if (UGP_GameInstance* GameInstance = GetGameInstance<UGP_GameInstance>())
	{
		GameInstance->ShowInitialOuterLoadingScreen();
	}

	if (IsValid(LobbyWidget))
	{
		LobbyWidget->ShowLoadingState();
	}
}

void AGP_LobbyPlayerController::ClientHideLoading_Implementation()
{
	if (UGP_GameInstance* GameInstance = GetGameInstance<UGP_GameInstance>())
	{
		GameInstance->HideInitialOuterLoadingScreen();
	}

	if (IsValid(LobbyWidget))
	{
		LobbyWidget->HideLoadingState();
	}

	SetInputMode(FInputModeUIOnly());
	SetShowMouseCursor(true);
}

void AGP_LobbyPlayerController::ServerForceStart_Implementation()
{
	UWorld* World = GetWorld();
	if (AGP_LobbyGameMode* GM = World ? World->GetAuthGameMode<AGP_LobbyGameMode>() : nullptr)
	{
		// The GameMode validates Shipping state, authority, the explicit launch
		// flag, and whether this is a local host or dedicated debug server.
		GM->TryForceStartGame(this);
	}
}
