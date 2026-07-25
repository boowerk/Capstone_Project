#include "Game/GP_GameMode.h"

#include "EngineUtils.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"

void AGP_GameMode::EnsurePartyPlayerStarts(AController* Player)
{
	if (bPartyPlayerStartsInitialized)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	TArray<APlayerStart*> AuthoredStarts;
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		if (IsValid(*It) && !It->Tags.Contains(TEXT("GP.RuntimePartyStart")))
		{
			AuthoredStarts.Add(*It);
		}
	}
	AuthoredStarts.Sort([](const APlayerStart& Left, const APlayerStart& Right)
	{
		// Actor paths are serialized and provide deterministic slot order across server runs.
		return Left.GetPathName() < Right.GetPathName();
	});

	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	const APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;

	if (AuthoredStarts.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[NetworkSpawn] No authored PlayerStart is available for the three-player runtime expansion."));
		bPartyPlayerStartsInitialized = true;
		return;
	}

	for (APlayerStart* AuthoredStart : AuthoredStarts)
	{
		if (PartyPlayerStartSlots.Num() >= RequiredPartyPlayerStartCount)
		{
			break;
		}
		PartyPlayerStartSlots.Add(AuthoredStart);
	}

	const APlayerStart& Anchor = *AuthoredStarts[0];
	static const FVector CandidateDirections[] =
	{
		FVector(0.0f, 1.0f, 0.0f),
		FVector(0.0f, -1.0f, 0.0f),
		FVector(1.0f, 0.0f, 0.0f),
		FVector(-1.0f, 0.0f, 0.0f),
		FVector(1.0f, 1.0f, 0.0f).GetSafeNormal(),
		FVector(1.0f, -1.0f, 0.0f).GetSafeNormal(),
		FVector(-1.0f, 1.0f, 0.0f).GetSafeNormal(),
		FVector(-1.0f, -1.0f, 0.0f).GetSafeNormal()
	};

	for (float RadiusMultiplier = 1.0f;
		PartyPlayerStartSlots.Num() < RequiredPartyPlayerStartCount && RadiusMultiplier <= 3.0f;
		RadiusMultiplier += 1.0f)
	{
		for (const FVector& Direction : CandidateDirections)
		{
			if (APlayerStart* RuntimeStart = SpawnRuntimePartyPlayerStart(
				Anchor,
				PawnToFit,
				Direction,
				RadiusMultiplier))
			{
				RuntimePartyPlayerStarts.Add(RuntimeStart);
				PartyPlayerStartSlots.Add(RuntimeStart);
			}

			if (PartyPlayerStartSlots.Num() >= RequiredPartyPlayerStartCount)
			{
				break;
			}
		}
	}

	bPartyPlayerStartsInitialized = true;
	if (PartyPlayerStartSlots.Num() < RequiredPartyPlayerStartCount)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("[NetworkSpawn] Only %d/%d collision-safe party starts could be prepared; engine fallback remains enabled."),
			PartyPlayerStartSlots.Num(),
			RequiredPartyPlayerStartCount);
	}
	else
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("[NetworkSpawn] Prepared %d stable party starts (%d authored, %d runtime)."),
			PartyPlayerStartSlots.Num(),
			FMath::Min(AuthoredStarts.Num(), RequiredPartyPlayerStartCount),
			RuntimePartyPlayerStarts.Num());
	}
}

APlayerStart* AGP_GameMode::SpawnRuntimePartyPlayerStart(
	const APlayerStart& Anchor,
	const APawn* PawnToFit,
	const FVector& LocalDirection,
	float RadiusMultiplier)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return nullptr;
	}

	const FVector WorldDirection =
		Anchor.GetActorForwardVector() * LocalDirection.X
		+ Anchor.GetActorRightVector() * LocalDirection.Y;
	FVector CandidateLocation =
		Anchor.GetActorLocation()
		+ WorldDirection.GetSafeNormal() * PartyPlayerStartSpacing * RadiusMultiplier;
	const FRotator CandidateRotation = Anchor.GetActorRotation();
	float MinimumWalkableNormalZ = 0.7f;
	if (const ACharacter* CharacterToFit = Cast<ACharacter>(PawnToFit))
	{
		if (const UCharacterMovementComponent* Movement = CharacterToFit->GetCharacterMovement())
		{
			MinimumWalkableNormalZ = Movement->GetWalkableFloorZ();
		}
	}

	const auto SnapToWalkableGround = [World, &Anchor, PawnToFit, MinimumWalkableNormalZ](FVector& InOutLocation)
	{
		FHitResult GroundHit;
		FCollisionQueryParams GroundQueryParams(SCENE_QUERY_STAT(GP_RuntimePartyStartGround), false);
		GroundQueryParams.AddIgnoredActor(&Anchor);
		const FVector TraceStart(InOutLocation.X, InOutLocation.Y, Anchor.GetActorLocation().Z + 1000.0f);
		const FVector TraceEnd(InOutLocation.X, InOutLocation.Y, Anchor.GetActorLocation().Z - 2000.0f);
		if (!World->LineTraceSingleByObjectType(
			GroundHit,
			TraceStart,
			TraceEnd,
			FCollisionObjectQueryParams(ECC_WorldStatic),
			GroundQueryParams)
			|| GroundHit.ImpactNormal.Z < MinimumWalkableNormalZ)
		{
			return false;
		}

		const float PawnHalfHeight = PawnToFit ? PawnToFit->GetDefaultHalfHeight() : 96.0f;
		InOutLocation.Z = GroundHit.ImpactPoint.Z + PawnHalfHeight + 2.0f;
		return true;
	};

	// Never register a start over void or on a slope the production character cannot walk on.
	if (!SnapToWalkableGround(CandidateLocation))
	{
		return nullptr;
	}

	if (PawnToFit
		&& World->EncroachingBlockingGeometry(PawnToFit, CandidateLocation, CandidateRotation)
		&& !World->FindTeleportSpot(PawnToFit, CandidateLocation, CandidateRotation))
	{
		return nullptr;
	}

	// FindTeleportSpot may move XY as well as Z; validate its final surface instead of trusting the first trace.
	if (!SnapToWalkableGround(CandidateLocation)
		|| (PawnToFit && World->EncroachingBlockingGeometry(PawnToFit, CandidateLocation, CandidateRotation)))
	{
		return nullptr;
	}

	const float MinimumSeparation = FMath::Max(150.0f, PartyPlayerStartSpacing * 0.6f);
	for (const TWeakObjectPtr<APlayerStart>& ExistingStart : PartyPlayerStartSlots)
	{
		if (ExistingStart.IsValid()
			&& FVector::DistSquared(ExistingStart->GetActorLocation(), CandidateLocation)
				< FMath::Square(MinimumSeparation))
		{
			return nullptr;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.ObjectFlags |= RF_Transient;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	APlayerStart* RuntimeStart = World->SpawnActor<APlayerStart>(
		APlayerStart::StaticClass(),
		CandidateLocation,
		CandidateRotation,
		SpawnParameters);
	if (IsValid(RuntimeStart))
	{
		// PlayerStarts are server-only selection helpers and never need to replicate to clients.
		RuntimeStart->SetReplicates(false);
		RuntimeStart->Tags.AddUnique(TEXT("GP.RuntimePartyStart"));
	}
	return RuntimeStart;
}

int32 AGP_GameMode::ResolvePartyStartSlot(AController* Player)
{
	if (!IsValid(Player))
	{
		return INDEX_NONE;
	}

	const TWeakObjectPtr<AController> PlayerKey(Player);
	for (auto It = PartyStartSlotByController.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}

	if (const int32* ExistingSlot = PartyStartSlotByController.Find(PlayerKey))
	{
		return *ExistingSlot;
	}

	TSet<int32> UsedSlots;
	for (const TPair<TWeakObjectPtr<AController>, int32>& Assignment : PartyStartSlotByController)
	{
		if (Assignment.Key.IsValid())
		{
			UsedSlots.Add(Assignment.Value);
		}
	}

	// Visual slots must not depend on runtime PlayerStart creation. Maps without a valid
	// authored start still use the engine spawn fallback, but 2P/3P must retain their
	// deterministic appearances.
	const int32 PartySlotCount = FMath::Clamp(RequiredPartyPlayerStartCount, 1, 3);
	for (int32 SlotIndex = 0; SlotIndex < PartySlotCount; ++SlotIndex)
	{
		if (!UsedSlots.Contains(SlotIndex))
		{
			PartyStartSlotByController.Add(PlayerKey, SlotIndex);
			return SlotIndex;
		}
	}
	return INDEX_NONE;
}
