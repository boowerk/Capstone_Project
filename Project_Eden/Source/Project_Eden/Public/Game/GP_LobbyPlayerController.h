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

	// Debug/solo start. Triggered by the lobby UI's force-start button; runs on
	// the server and bypasses the lobby ready/count gate.
	UFUNCTION(Server, Reliable)
	void ServerForceStart();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UGP_LobbyWidget> LobbyWidgetClass;

private:
#if !UE_BUILD_SHIPPING
	// Headless QA uses the real Ready RPC; this retry only waits for the
	// replicated PlayerState when -LobbySmokeAutoReady is explicitly supplied.
	void BeginLobbySmokeAutoReady();
	void TryLobbySmokeAutoReady();
	FTimerHandle LobbySmokeReadyTimerHandle;
	int32 LobbySmokeReadyAttempts = 0;
#endif

	UPROPERTY()
	TObjectPtr<UGP_LobbyWidget> LobbyWidget;
};
