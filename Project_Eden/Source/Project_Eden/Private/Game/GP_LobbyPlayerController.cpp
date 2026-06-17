#include "Game/GP_LobbyPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "UI/GP_LobbyWidget.h"

void AGP_LobbyPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || !LobbyWidgetClass)
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
