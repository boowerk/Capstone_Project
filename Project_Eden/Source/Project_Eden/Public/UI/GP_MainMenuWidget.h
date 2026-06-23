#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_MainMenuWidget.generated.h"

class UButton;
class UEditableTextBox;

/**
 * Main menu widget. BP child places the actual widgets; C++ handles all logic.
 *
 * Required widget names in BP:
 *   Button_Host       (UButton)        — start a listen server and open LobbyMap
 *   Button_Join       (UButton)        — join the IP typed in TextBox_IP
 *   Button_Quit       (UButton)        — quit the game
 *   TextBox_IP        (UEditableTextBox) — IP address input (default "127.0.0.1")
 */
UCLASS()
class PROJECT_EDEN_API UGP_MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// Map name for the lobby (must match the actual asset name under /Game/Maps/).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Menu")
	FString LobbyMapName = TEXT("MainMap/LobbyMap");

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Host;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Join;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> Button_Quit;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UEditableTextBox> TextBox_IP;

	UFUNCTION()
	void OnHostClicked();

	UFUNCTION()
	void OnJoinClicked();

	UFUNCTION()
	void OnQuitClicked();
};
