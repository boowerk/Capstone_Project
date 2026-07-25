#include "Game/GP_GameMode.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Game/GP_GameState.h"
#include "Game/GP_RunProgressionPolicy.h"
#include "GameFramework/PlayerState.h"
#include "Player/GP_PlayerState.h"
#include "TimerManager.h"

void AGP_GameMode::NotifyAllPlayersDead()
{
	FinishRun(/*bVictory=*/false);
}

void AGP_GameMode::NotifyPlayerEliminated(AGP_PlayerState* EliminatedPlayerState)
{
	if (bRunFinished
		|| !IsValid(EliminatedPlayerState)
		|| !EliminatedPlayerState->IsEliminated())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				EvaluatePartyDefeat();
			}));
	}
}

void AGP_GameMode::EvaluatePartyDefeat()
{
	if (bRunFinished)
	{
		return;
	}

	const AGP_GameState* GPGameState = GetGPGameState();
	if (!IsValid(GPGameState))
	{
		return;
	}

	int32 ParticipantCount = 0;
	int32 EliminatedParticipantCount = 0;
	for (APlayerState* PartyPlayerState : GPGameState->PlayerArray)
	{
		if (!IsValid(PartyPlayerState) || PartyPlayerState->IsOnlyASpectator())
		{
			continue;
		}

		const AGP_PlayerState* GPPlayerState = Cast<AGP_PlayerState>(PartyPlayerState);
		if (!IsValid(GPPlayerState))
		{
			continue;
		}

		if (GPPlayerState->IsEliminated())
		{
			// An eliminated member remains part of the run even if their pawn is
			// later destroyed while the final party decision is being made.
			++ParticipantCount;
			++EliminatedParticipantCount;
			continue;
		}

		// A connecting PlayerState can replicate before possession. Do not let
		// that temporary pawn-less slot masquerade as a survivor and prevent an
		// otherwise valid dedicated-server party wipe.
		if (IsValid(Cast<AGP_PlayerCharacter>(GPPlayerState->GetPawn())))
		{
			++ParticipantCount;
		}
	}

	if (GPRunProgressionPolicy::IsPartyDefeated(
		ParticipantCount,
		EliminatedParticipantCount))
	{
		NotifyAllPlayersDead();
	}
}

void AGP_GameMode::FinishRun(bool bVictory)
{
	if (bRunFinished)
	{
		return;
	}

	bRunFinished = true;
	UnregisterAllZoneNavigationInvokers();

	if (AGP_GameState* GPGameState = GetGPGameState())
	{
		GPGameState->SetMatchPhase(bVictory ? EGPMatchPhase::Victory : EGPMatchPhase::Defeat);
	}

	OnRunFinished(bVictory);

	// Send the party back to the lobby after a beat so the result screen shows.
	if (UWorld* World = GetWorld())
	{
		if (ReturnToLobbyDelay > 0.0f)
		{
			World->GetTimerManager().SetTimer(ReturnToLobbyTimerHandle, this,
				&AGP_GameMode::ReturnToLobby, ReturnToLobbyDelay, false);
		}
		else
		{
			ReturnToLobby();
		}
	}
}

void AGP_GameMode::ReturnToLobby()
{
	if (UWorld* World = GetWorld())
	{
		// Seamless ServerTravel carries every connected client back together.
		const FString URL = FString::Printf(TEXT("/Game/Maps/%s?listen"), *ReturnMapName);
		World->ServerTravel(URL);
	}
}
