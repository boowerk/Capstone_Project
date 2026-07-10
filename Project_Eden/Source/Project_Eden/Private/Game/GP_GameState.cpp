#include "Game/GP_GameState.h"

#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Net/UnrealNetwork.h"

AGP_GameState::AGP_GameState()
{
	// A replicated default subobject keeps corruption available to every GameState client without a map actor dependency.
	WorldCorruptionComponent = CreateDefaultSubobject<UGP_WorldCorruptionComponent>(TEXT("WorldCorruptionComponent"));
}

void AGP_GameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_GameState, MatchPhase);
	DOREPLIFETIME(AGP_GameState, CurrentZoneIndex);
	DOREPLIFETIME(AGP_GameState, EnemiesRemaining);
	DOREPLIFETIME(AGP_GameState, RegionStates);
}

uint8 AGP_GameState::GetRegionState(int32 RegionId) const
{
	return RegionStates.IsValidIndex(RegionId) ? RegionStates[RegionId] : 0;
}

void AGP_GameState::InitRegionStates(int32 Count, uint8 InitialState)
{
	if (!HasAuthority() || Count < 0)
	{
		return;
	}

	RegionStates.Init(InitialState, Count);
	LocalRegionStatesShadow = RegionStates;

	// Server gets no OnRep; broadcast a full reset locally for the listen host.
	OnRegionStateChanged.Broadcast(INDEX_NONE, InitialState);
}

void AGP_GameState::SetRegionState(int32 RegionId, uint8 NewState)
{
	if (!HasAuthority() || !RegionStates.IsValidIndex(RegionId) || RegionStates[RegionId] == NewState)
	{
		return;
	}

	RegionStates[RegionId] = NewState;
	LocalRegionStatesShadow = RegionStates;
	OnRegionStateChanged.Broadcast(RegionId, NewState);
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

void AGP_GameState::OnRep_RegionStates()
{
	// Detect what changed versus the last applied snapshot. A size change (or first
	// replication) means a full (re)init, so broadcast a reset; otherwise broadcast
	// each individual region that flipped so only those regenerate.
	if (LocalRegionStatesShadow.Num() != RegionStates.Num())
	{
		LocalRegionStatesShadow = RegionStates;
		OnRegionStateChanged.Broadcast(INDEX_NONE, RegionStates.Num() > 0 ? RegionStates[0] : 0);
		return;
	}

	for (int32 RegionId = 0; RegionId < RegionStates.Num(); ++RegionId)
	{
		if (LocalRegionStatesShadow[RegionId] != RegionStates[RegionId])
		{
			OnRegionStateChanged.Broadcast(RegionId, RegionStates[RegionId]);
		}
	}

	LocalRegionStatesShadow = RegionStates;
}
