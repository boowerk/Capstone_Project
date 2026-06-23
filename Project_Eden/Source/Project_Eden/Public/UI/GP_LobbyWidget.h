#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_LobbyWidget.generated.h"

class UButton;
class UTextBlock;
class UVerticalBox;
class AGP_LobbyPlayerState;
class UGP_LobbyPlayerRowWidget;

/**
 * Lobby widget shown while waiting for all players to ready up.
 *
 * Required widget names in BP:
 *   Button_Ready      (UButton)      — toggles ready state
 *   Text_PlayerCount  (UTextBlock)   — shows "X / Y"
 *   Box_PlayerList    (UVerticalBox) — populated at runtime with player rows
 *   Button_Leave      (UButton, optional) — leaves the lobby back to main menu
 *   Button_ForceStart (UButton, optional) — debug/solo: starts the run alone
 */
UCLASS()
class PROJECT_EDEN_API UGP_LobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void NotifyPlayerListChanged();

	// Called by the server just before ServerTravel. Locks the UI and shows
	// "로딩중" for all players so the last ready-state update is never missed.
	void ShowLoadingState();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Widget class used for each player row inside Box_PlayerList.
	// Create a simple BP widget with Text_Name (UTextBlock) + Text_Status (UTextBlock).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	TSubclassOf<UGP_LobbyPlayerRowWidget> PlayerRowWidgetClass;

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Ready;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_PlayerCount;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> Box_PlayerList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Leave;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_ForceStart;

	bool bIsReady = false;
	bool bIsLoading = false;

	UFUNCTION()
	void OnReadyClicked();

	UFUNCTION()
	void OnLeaveClicked();

	UFUNCTION()
	void OnForceStartClicked();

	// Rebuilds Box_PlayerList from current GameState.PlayerArray.
	void RefreshPlayerList();

	void BindToPlayerStates();
	void UnbindFromPlayerStates();

	UFUNCTION()
	void OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bReady);

	// PlayerArray replicates to clients with no change notification, so poll it
	// on a timer to pick up players that join after this widget was constructed.
	FTimerHandle RefreshTimerHandle;

	// Tracks bound player states so we can unbind on destruct.
	UPROPERTY()
	TArray<TObjectPtr<AGP_LobbyPlayerState>> BoundPlayerStates;
};
