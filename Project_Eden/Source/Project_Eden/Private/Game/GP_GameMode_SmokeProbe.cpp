#include "Game/GP_GameMode.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/GP_PlayerController.h"
#include "Player/GP_PlayerState.h"
#include "TimerManager.h"

#if !UE_BUILD_SHIPPING
void AGP_GameMode::BeginThreePlayerGameplaySmokeProbe()
{
	UWorld* World = GetWorld();
	if (!IsValid(World)
		|| !World->GetMapName().Contains(TEXT("L_LandscapeMap"))
		|| !FParse::Param(FCommandLine::Get(), TEXT("LobbySmokeAutoReady")))
	{
		return;
	}

	ThreePlayerGameplaySmokeAttempts = 0;
	World->GetTimerManager().SetTimer(
		ThreePlayerGameplaySmokeTimerHandle,
		this,
		&ThisClass::TryThreePlayerGameplaySmokeProbe,
		0.25f,
		false);
}

void AGP_GameMode::TryThreePlayerGameplaySmokeProbe()
{
	++ThreePlayerGameplaySmokeAttempts;
	constexpr int32 ExpectedPlayers = 3;
	constexpr int32 MaxAttempts = 80;
	constexpr float MinimumPawnSeparation = 150.0f;

	TArray<APlayerController*> Controllers;
	TArray<APawn*> Pawns;
	TSet<APawn*> UniquePawns;
	int32 OwnedPawnCount = 0;
	int32 GameplayControllerCount = 0;
	int32 GameplayPawnCount = 0;
	int32 GameplayPlayerStateCount = 0;

	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			APlayerController* Controller = It->Get();
			if (!IsValid(Controller))
			{
				continue;
			}

			Controllers.Add(Controller);
			GameplayControllerCount += Controller->IsA<AGP_PlayerController>() ? 1 : 0;
			GameplayPlayerStateCount += Controller->GetPlayerState<AGP_PlayerState>() ? 1 : 0;
			if (APawn* Pawn = Controller->GetPawn())
			{
				Pawns.Add(Pawn);
				UniquePawns.Add(Pawn);
				OwnedPawnCount += Pawn->GetController() == Controller ? 1 : 0;
				GameplayPawnCount += Pawn->IsA<AGP_PlayerCharacter>() ? 1 : 0;
			}
		}
	}

	float MinimumObservedSeparation = TNumericLimits<float>::Max();
	for (int32 FirstIndex = 0; FirstIndex < Pawns.Num(); ++FirstIndex)
	{
		for (int32 SecondIndex = FirstIndex + 1; SecondIndex < Pawns.Num(); ++SecondIndex)
		{
			MinimumObservedSeparation = FMath::Min(
				MinimumObservedSeparation,
				FVector::Dist(Pawns[FirstIndex]->GetActorLocation(), Pawns[SecondIndex]->GetActorLocation()));
		}
	}
	if (Pawns.Num() < 2)
	{
		MinimumObservedSeparation = 0.0f;
	}

	const bool bGameplayReady =
		Controllers.Num() == ExpectedPlayers
		&& Pawns.Num() == ExpectedPlayers
		&& UniquePawns.Num() == ExpectedPlayers
		&& OwnedPawnCount == ExpectedPlayers
		&& GameplayControllerCount == ExpectedPlayers
		&& GameplayPawnCount == ExpectedPlayers
		&& GameplayPlayerStateCount == ExpectedPlayers
		&& MinimumObservedSeparation >= MinimumPawnSeparation;

	if (bGameplayReady)
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[Network3PSmoke] server-gameplay-ready controllers=%d pawns=%d unique=%d owned=%d gamePC=%d gamePawn=%d gamePS=%d minSeparation=%.1f"),
			Controllers.Num(),
			Pawns.Num(),
			UniquePawns.Num(),
			OwnedPawnCount,
			GameplayControllerCount,
			GameplayPawnCount,
			GameplayPlayerStateCount,
			MinimumObservedSeparation);
		return;
	}

	if (ThreePlayerGameplaySmokeAttempts >= MaxAttempts)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[Network3PSmoke] server-gameplay-timeout attempts=%d controllers=%d pawns=%d unique=%d owned=%d gamePC=%d gamePawn=%d gamePS=%d minSeparation=%.1f"),
			ThreePlayerGameplaySmokeAttempts,
			Controllers.Num(),
			Pawns.Num(),
			UniquePawns.Num(),
			OwnedPawnCount,
			GameplayControllerCount,
			GameplayPawnCount,
			GameplayPlayerStateCount,
			MinimumObservedSeparation);
		return;
	}

	GetWorldTimerManager().SetTimer(
		ThreePlayerGameplaySmokeTimerHandle,
		this,
		&ThisClass::TryThreePlayerGameplaySmokeProbe,
		0.25f,
		false);
}
#endif
