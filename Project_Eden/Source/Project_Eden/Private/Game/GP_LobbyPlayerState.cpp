#include "Game/GP_LobbyPlayerState.h"

#include "Net/UnrealNetwork.h"

void AGP_LobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_LobbyPlayerState, bReady);
}

void AGP_LobbyPlayerState::ServerSetReady_Implementation(bool bNewReady)
{
	if (bReady == bNewReady)
	{
		return;
	}

	bReady = bNewReady;
	OnReadyChanged.Broadcast(this, bReady);
}

void AGP_LobbyPlayerState::OnRep_bReady()
{
	OnReadyChanged.Broadcast(this, bReady);
}
