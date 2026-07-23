#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameSession.h"
#include "GP_ThreePlayerGameSession.generated.h"

/**
 * Server-side admission policy for Project Eden's fixed three-player party.
 *
 * The limits are reapplied after URL option parsing so a launch option such as
 * ?MaxPlayers=4 cannot silently loosen the production contract.
 */
UCLASS(Config = Game)
class PROJECT_EDEN_API AGP_ThreePlayerGameSession : public AGameSession
{
	GENERATED_BODY()

public:
	static constexpr int32 PartyPlayerCapacity = 3;
	static constexpr int32 PartySpectatorCapacity = 0;
	static constexpr uint8 PartySplitscreenCapacity = 1;

	AGP_ThreePlayerGameSession();

	virtual void InitOptions(const FString& Options) override;
	virtual bool AtCapacity(bool bSpectator) override;

private:
	void ApplyPartyLimits();
};
