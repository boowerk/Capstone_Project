#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GP_LobbyGameMode.generated.h"

class AGP_LobbyPlayerState;
class APlayerController;

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

	// The session cap is the primary admission guard; this exact comparison is
	// defense in depth so an over-capacity lobby can never begin travel.
	static bool HasExactPartySize(int32 ConnectedPlayerCount, int32 RequiredPlayerCount);

	// Debug/solo starts are disabled in Shipping and require both an explicit
	// launch flag and an authoritative local (listen/standalone) controller.
	bool CanForceStart(const APlayerController* RequestingController) const;
	bool TryForceStartGame(APlayerController* RequestingController);

	// Pure policy seam used to keep every caller aligned with the same
	// Shipping/authority/local-controller restrictions.
	static bool IsForceStartRequestAllowed(
		bool bExplicitlyEnabled,
		bool bHasAuthority,
		bool bIsLocalController);

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

	bool bTravelStarted = false;

	void BroadcastRefreshPlayerList();
	void BroadcastLoading();
	void TravelToGame();

	// Last Ready, Blueprint callbacks, and an allowed debug force-start can
	// arrive in the same frame; only the first path may initiate ServerTravel.
	bool bTravelInitiated = false;
};
