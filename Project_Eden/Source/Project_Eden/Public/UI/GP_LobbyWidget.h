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
 */
UCLASS()
class PROJECT_EDEN_API UGP_LobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void NotifyPlayerListChanged();

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

	bool bIsReady = false;

	UFUNCTION()
	void OnReadyClicked();

	// Rebuilds Box_PlayerList from current GameState.PlayerArray.
	void RefreshPlayerList();

	void BindToPlayerStates();
	void UnbindFromPlayerStates();

	UFUNCTION()
	void OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bReady);

	// Tracks bound player states so we can unbind on destruct.
	UPROPERTY()
	TArray<TObjectPtr<AGP_LobbyPlayerState>> BoundPlayerStates;
};
