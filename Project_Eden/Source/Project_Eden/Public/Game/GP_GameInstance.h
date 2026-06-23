#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/EngineBaseTypes.h"
#include "GP_GameInstance.generated.h"

/**
 * Handles connection lifecycle errors so a failed join or a dropped host never
 * leaves a client stuck on a dead map:
 *   - Join to a bad/unreachable IP -> network failure -> back to main menu.
 *   - Host quits / server shuts down -> clients get ConnectionLost -> back to menu.
 * The last error string is kept so the menu can show why the connection ended.
 */
UCLASS()
class PROJECT_EDEN_API UGP_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	// Travels the local player back to the main menu (disconnecting from any
	// server). Used by the lobby's Leave button and on connection errors.
	UFUNCTION(BlueprintCallable, Category = "Flow")
	void ReturnToMainMenu();

	// Last connection error message (empty if none). The menu can read and clear
	// this to display "Connection lost" / "Could not reach host" to the player.
	UFUNCTION(BlueprintPure, Category = "Flow")
	FString GetLastConnectionError() const { return LastConnectionError; }

	UFUNCTION(BlueprintCallable, Category = "Flow")
	void ClearLastConnectionError() { LastConnectionError.Empty(); }

protected:
	// Main menu level to fall back to. Must match the asset under /Game/Maps/.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flow")
	FString MainMenuMapName = TEXT("/Game/Maps/MainMap/MainMenuMap");

private:
	// Bound to GEngine multicast delegates via AddUObject (not reflected, so no
	// UFUNCTION — their namespaced-enum params aren't valid UFUNCTION types).
	void HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	// True for net modes where THIS instance is the authority (listen host or
	// dedicated server). Such instances must not travel themselves to the menu
	// just because a remote client dropped.
	bool IsAuthority(const UWorld* World) const;

	FString LastConnectionError;
};
