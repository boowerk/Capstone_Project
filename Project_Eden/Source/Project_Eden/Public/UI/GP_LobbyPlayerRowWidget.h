#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_LobbyPlayerRowWidget.generated.h"

class UTextBlock;

/**
 * One player row in the lobby list.
 * BP child (WBP_LobbyPlayerRow) must have:
 *   Text_Name   (UTextBlock)
 *   Text_Status (UTextBlock)
 */
UCLASS()
class PROJECT_EDEN_API UGP_LobbyPlayerRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetPlayerInfo(const FString& PlayerName, bool bReady);

private:
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Status;
};
