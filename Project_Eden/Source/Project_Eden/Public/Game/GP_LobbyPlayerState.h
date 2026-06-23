#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GP_LobbyPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnReadyChanged, AGP_LobbyPlayerState*, PlayerState, bool, bReady);

UCLASS()
class PROJECT_EDEN_API AGP_LobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Lobby")
	bool IsReady() const { return bReady; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Lobby")
	void ServerSetReady(bool bNewReady);

	UPROPERTY(BlueprintAssignable, Category = "Lobby")
	FGPOnReadyChanged OnReadyChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_bReady)
	bool bReady = false;

	UFUNCTION()
	void OnRep_bReady();
};
