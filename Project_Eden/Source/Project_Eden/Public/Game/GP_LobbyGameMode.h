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

	// 준비된 파티가 검증된 오픈월드 졸업작품 슬라이스로 함께 이동하도록 기본 목적지를 고정합니다.
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

	// Last Ready, Blueprint callbacks, and debug ForceStart can arrive in the
	// same frame; only the authoritative first path may initiate ServerTravel.
	bool bTravelInitiated = false;
};
