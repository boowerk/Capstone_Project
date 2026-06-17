#include "Game/GP_GameState.h"

#include "Net/UnrealNetwork.h"

AGP_GameState::AGP_GameState()
{
}

void AGP_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_GameState, MatchPhase);
	DOREPLIFETIME(AGP_GameState, CurrentZoneIndex);
	DOREPLIFETIME(AGP_GameState, EnemiesRemaining);
}

void AGP_GameState::SetMatchPhase(EGPMatchPhase NewPhase)
{
	if (!HasAuthority() || MatchPhase == NewPhase)
	{
		return;
	}

	MatchPhase = NewPhase;
	// The server does not receive OnRep, so broadcast locally to keep listen-server HUDs in sync.
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AGP_GameState::SetCurrentZoneIndex(int32 NewZoneIndex)
{
	if (!HasAuthority() || CurrentZoneIndex == NewZoneIndex)
	{
		return;
	}

	CurrentZoneIndex = NewZoneIndex;
}

void AGP_GameState::SetEnemiesRemaining(int32 NewEnemiesRemaining)
{
	NewEnemiesRemaining = FMath::Max(0, NewEnemiesRemaining);
	if (!HasAuthority() || EnemiesRemaining == NewEnemiesRemaining)
	{
		return;
	}

	EnemiesRemaining = NewEnemiesRemaining;
	OnEnemiesRemainingChanged.Broadcast(EnemiesRemaining);
}

void AGP_GameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

void AGP_GameState::OnRep_CurrentZoneIndex()
{
}

void AGP_GameState::OnRep_EnemiesRemaining()
{
	OnEnemiesRemainingChanged.Broadcast(EnemiesRemaining);
}
