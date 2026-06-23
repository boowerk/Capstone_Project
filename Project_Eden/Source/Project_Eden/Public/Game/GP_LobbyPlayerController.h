#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GP_LobbyPlayerController.generated.h"

class UGP_LobbyWidget;

UCLASS()
class PROJECT_EDEN_API AGP_LobbyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	UFUNCTION(Client, Reliable)
	void ClientRefreshPlayerList();

	UFUNCTION(Client, Reliable)
	void ClientShowLoading();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGP_LobbyWidget> LobbyWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UGP_LobbyWidget> LobbyWidget;
};
