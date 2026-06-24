#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_LobbyGameMode.generated.h"

class AGP_LobbyPlayerState;

UCLASS()
class PROJECT_EDEN_API AGP_LobbyGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AGP_LobbyGameMode();

	virtual void BeginPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;

	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void CheckAllReady();

	// Debug/solo: start the run immediately, ignoring player count and ready
	// state. Server-authoritative; invoked via the lobby controller's exec.
	UFUNCTION(BlueprintCallable, Category = "Lobby")
	void ForceStartGame();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	int32 ExpectedPlayerCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Lobby")
	FString GameMapName = TEXT("MainMap/L_LandscapeMap");

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnAllPlayersReady();

	UFUNCTION(BlueprintImplementableEvent, Category = "Lobby")
	void OnPlayerCountChanged(int32 NewCount);

private:
	UFUNCTION()
	void OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bIsReady);

	void BroadcastRefreshPlayerList();
	void BroadcastLoading();
	void TravelToGame();
};
