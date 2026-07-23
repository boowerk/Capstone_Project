#include "Game/GP_ThreePlayerGameSession.h"

#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"

AGP_ThreePlayerGameSession::AGP_ThreePlayerGameSession()
{
	ApplyPartyLimits();
}

void AGP_ThreePlayerGameSession::InitOptions(const FString& Options)
{
	Super::InitOptions(Options);

	// URL options are parsed by the base class, so restore the fixed product
	// limits afterward instead of trusting deployment-specific launch flags.
	ApplyPartyLimits();
}

bool AGP_ThreePlayerGameSession::AtCapacity(bool bSpectator)
{
	if (GetNetMode() == NM_Standalone)
	{
		return false;
	}

	// Project Eden has no network spectator role; every accepted connection
	// must own one of the three gameplay player slots.
	if (bSpectator)
	{
		return true;
	}

	UWorld* World = GetWorld();
	AGameModeBase* GameMode = World ? World->GetAuthGameMode() : nullptr;

	// Fail closed while the authoritative GameMode is unavailable so a
	// transient initialization gap cannot admit an uncounted connection.
	return !GameMode || GameMode->GetNumPlayers() >= PartyPlayerCapacity;
}

void AGP_ThreePlayerGameSession::ApplyPartyLimits()
{
	MaxPlayers = PartyPlayerCapacity;
	MaxSpectators = PartySpectatorCapacity;
	MaxPartySize = PartyPlayerCapacity;
	MaxSplitscreensPerConnection = PartySplitscreenCapacity;
}
