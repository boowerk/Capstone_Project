#include "Game/GP_LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "UI/GP_LobbyWidget.h"

void AGP_LobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

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

void AGP_LobbyPlayerController::ClientRefreshPlayerList_Implementation()
{
	if (IsValid(LobbyWidget))
	{
		LobbyWidget->NotifyPlayerListChanged();
	}
}
