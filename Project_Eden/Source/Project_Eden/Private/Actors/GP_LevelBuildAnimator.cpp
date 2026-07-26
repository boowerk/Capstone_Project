#include "Actors/GP_LevelBuildAnimator.h"

#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Components/SceneComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

AGP_LevelBuildAnimator::AGP_LevelBuildAnimator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Tags.AddUnique(TEXT("LevelBuildAnimator"));
}

void AGP_LevelBuildAnimator::BeginPlay()
{
	Super::BeginPlay();

	// A Blueprint child created before native replication was enabled can retain
	// its old CDO value. Enforce the authoritative runtime contract for the
	// map-placed Colosseum animator.
	if (HasServerAuthority())
	{
		bAlwaysRelevant = true;
		SetReplicates(true);
	}

	RebuildPieceList();

	if (bWaitForColosseumPortalArrival)
	{
		// Move authority collision and client visuals together. Keeping final
		// collision only on a dedicated server would fight client prediction.
		ResetBuild(/*bMovePiecesToStart=*/true);
		ApplyPlaybackSnapshot();
	}
	else if (bAutoStartOnBeginPlay)
	{
		StartBuild();
	}
}

void AGP_LevelBuildAnimator::GetLifetimeReplicatedProps(
	TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_LevelBuildAnimator, PlaybackSnapshot);
}

void AGP_LevelBuildAnimator::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsBuilding)
	{
		return;
	}

	const float PreviousBuildTime = BuildTime;
	if (bUseSynchronizedBuildClock
		&& PlaybackSnapshot.Phase == EGPLevelBuildPlaybackPhase::Playing)
	{
		const float SynchronizedBuildTime = static_cast<float>(FMath::Max(
			0.0,
			GetSynchronizedWorldTimeSeconds() - PlaybackSnapshot.StartServerTime));
		BuildTime = FMath::Max(
			BuildTime,
			FMath::Min(SynchronizedBuildTime, PlaybackSnapshot.Duration));
	}
	else
	{
		BuildTime += DeltaSeconds;
	}

	bool bAllFinished = true;
	for (int32 PieceIndex = 0; PieceIndex < Pieces.Num(); ++PieceIndex)
	{
		FGPLevelBuildPiece& Piece = Pieces[PieceIndex];
		if (Piece.bFinished)
		{
			continue;
		}

		if (!IsValid(Piece.Actor))
		{
			Piece.bFinished = true;
			continue;
		}

		const float RawLocalAlpha = (BuildTime - Piece.StartTime) / FMath::Max(KINDA_SMALL_NUMBER, Piece.Duration);
		if (RawLocalAlpha < 0.0f)
		{
			bAllFinished = false;
			continue;
		}

		if (PreviousBuildTime <= Piece.StartTime && BuildTime > Piece.StartTime)
		{
			BP_OnBuildPieceStarted(Piece.Actor, PieceIndex);
		}

		if (RawLocalAlpha >= 1.0f)
		{
			Piece.Actor->SetActorTransform(Piece.FinalTransform, false, nullptr, ETeleportType::TeleportPhysics);
			Piece.bFinished = true;
			BP_OnBuildPieceFinished(Piece.Actor, PieceIndex);
		}
		else
		{
			ApplyPieceTransform(Piece, FMath::Max(0.0f, RawLocalAlpha));
			bAllFinished = false;
		}
	}

	if (BuildTime >= TotalBuildTime && bAllFinished)
	{
		FinishBuild();
	}
}

void AGP_LevelBuildAnimator::RebuildPieceList()
{
	Pieces.Reset();

	FRandomStream RandomStream(RandomSeed);
	const FVector PivotLocation = ResolvePivotLocation();

	TArray<AActor*> Candidates;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		AActor* Candidate = *It;
		if (!ShouldControlActor(Candidate))
		{
			continue;
		}
		Candidates.Add(Candidate);
	}

	// Every machine must assign the same deterministic random values to the same
	// authored pieces before applying the replicated start clock.
	Candidates.Sort([](const AActor& Left, const AActor& Right)
	{
		return Left.GetPathName() < Right.GetPathName();
	});

	for (AActor* Candidate : Candidates)
	{
		FGPLevelBuildPiece Piece;
		Piece.Actor = Candidate;
		Piece.FinalTransform = Candidate->GetActorTransform();
		Piece.HeightKey = FMath::GridSnap(Piece.FinalTransform.GetLocation().Z, SameHeightTolerance);
		Piece.DistanceKey = FVector::Dist2D(Piece.FinalTransform.GetLocation(), PivotLocation);
		Piece.RandomValue = RandomStream.FRand();
		Piece.EffectiveOrderMode = ResolveOrderModeForActor(Candidate);
		Piece.PriorityBand = ResolvePriorityBandForActor(Candidate);

		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		Candidate->GetActorBounds(false, BoundsOrigin, BoundsExtent);
		Piece.BoundsRadius = BoundsExtent.Size();
		Pieces.Add(Piece);
	}

	BuildSortKeys();
	SortPieces();
	PrepareStartTransforms();
	TotalBuildTime = CalculateTotalBuildTime();
}

void AGP_LevelBuildAnimator::StartBuild()
{
	bUseSynchronizedBuildClock = false;
	StartBuildAtElapsedTime(
		0.0f,
		/*bNotifyBuildStarted=*/true);
}

void AGP_LevelBuildAnimator::StartBuildAtElapsedTime(
	float ElapsedSeconds,
	bool bNotifyBuildStarted)
{
	if (Pieces.Num() == 0)
	{
		RebuildPieceList();
	}

	if (Pieces.Num() == 0)
	{
		return;
	}

	BuildTime = FMath::Clamp(ElapsedSeconds, 0.0f, TotalBuildTime);
	bIsBuilding = BuildTime < TotalBuildTime;
	SetActorTickEnabled(true);

	if (bNotifyBuildStarted)
	{
		BP_OnBuildStarted();
	}

	for (int32 PieceIndex = 0; PieceIndex < Pieces.Num(); ++PieceIndex)
	{
		FGPLevelBuildPiece& Piece = Pieces[PieceIndex];
		Piece.bFinished = false;
		if (IsValid(Piece.Actor))
		{
			Piece.Actor->SetActorHiddenInGame(false);
			if (BuildTime <= Piece.StartTime)
			{
				Piece.Actor->SetActorTransform(
					Piece.StartTransform,
					false,
					nullptr,
					ETeleportType::TeleportPhysics);
				continue;
			}

			const float LocalAlpha = FMath::Clamp(
				(BuildTime - Piece.StartTime)
					/ FMath::Max(KINDA_SMALL_NUMBER, Piece.Duration),
				0.0f,
				1.0f);
			// A remote client normally receives the replicated start a few
			// frames late. Start effects for pieces that are still active, but
			// do not replay effects for every piece a late joiner already missed.
			if (bNotifyBuildStarted && LocalAlpha < 1.0f)
			{
				BP_OnBuildPieceStarted(Piece.Actor, PieceIndex);
			}
			ApplyPieceTransform(Piece, LocalAlpha);
			Piece.bFinished = LocalAlpha >= 1.0f;
		}
	}

	if (!bIsBuilding)
	{
		FinishBuild();
	}
}

void AGP_LevelBuildAnimator::ResetBuild(bool bMovePiecesToStart)
{
	bUseSynchronizedBuildClock = false;
	BuildTime = 0.0f;
	bIsBuilding = false;
	SetActorTickEnabled(false);

	for (FGPLevelBuildPiece& Piece : Pieces)
	{
		Piece.bFinished = false;
		if (bMovePiecesToStart && IsValid(Piece.Actor))
		{
			Piece.Actor->SetActorTransform(Piece.StartTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
	}
}

void AGP_LevelBuildAnimator::FinishBuild()
{
	for (FGPLevelBuildPiece& Piece : Pieces)
	{
		if (!Piece.bFinished && IsValid(Piece.Actor))
		{
			Piece.Actor->SetActorTransform(Piece.FinalTransform, false, nullptr, ETeleportType::TeleportPhysics);
		}
		Piece.bFinished = true;
	}

	const bool bWasBuilding = bIsBuilding;
	bIsBuilding = false;
	bUseSynchronizedBuildClock = false;
	SetActorTickEnabled(false);

	if (bWasBuilding)
	{
		BP_OnBuildFinished();
	}

	if (HasServerAuthority()
		&& PlaybackSnapshot.Phase == EGPLevelBuildPlaybackPhase::Playing)
	{
		MarkBuildFinishedAuthoritative();
	}
}

float AGP_LevelBuildAnimator::GetBuildAlpha() const
{
	return TotalBuildTime > KINDA_SMALL_NUMBER ? FMath::Clamp(BuildTime / TotalBuildTime, 0.0f, 1.0f) : 1.0f;
}

bool AGP_LevelBuildAnimator::ShouldControlActor(const AActor* Candidate) const
{
	if (!IsValid(Candidate) || Candidate == this)
	{
		return false;
	}
	if (Candidate->GetLevel() != GetLevel())
	{
		return false;
	}

	if (!bIncludeHiddenActors && Candidate->IsHidden())
	{
		return false;
	}

	if (!Candidate->ActorHasTag(TargetActorTag))
	{
		return false;
	}

	if (!bIncludeSelfAttachedActors && Candidate->IsAttachedTo(this))
	{
		return false;
	}

	return true;
}

bool AGP_LevelBuildAnimator::HasServerAuthority() const
{
	return GetNetMode() != NM_Client && HasAuthority();
}

bool AGP_LevelBuildAnimator::StartBuildAuthoritative()
{
	if (!HasServerAuthority()
		|| PlaybackSnapshot.Phase != EGPLevelBuildPlaybackPhase::Waiting)
	{
		return false;
	}

	if (Pieces.IsEmpty())
	{
		RebuildPieceList();
	}
	if (Pieces.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LevelBuildAnimator] Cannot start '%s': no actors use target tag '%s'."),
			*GetName(),
			*TargetActorTag.ToString());
		return false;
	}

	PlaybackSnapshot.Revision = FMath::Max(1, PlaybackSnapshot.Revision + 1);
	PlaybackSnapshot.Phase = EGPLevelBuildPlaybackPhase::Playing;
	PlaybackSnapshot.StartServerTime = GetSynchronizedWorldTimeSeconds();
	PlaybackSnapshot.Duration = TotalBuildTime;
	ForceNetUpdate();

	ApplyPlaybackSnapshot();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			AuthoritativeBuildTimerHandle,
			this,
			&ThisClass::HandleAuthoritativeBuildTimerExpired,
			FMath::Max(0.01f, PlaybackSnapshot.Duration),
			false);
	}

	return true;
}

void AGP_LevelBuildAnimator::OnRep_PlaybackSnapshot()
{
	ApplyPlaybackSnapshot();
}

void AGP_LevelBuildAnimator::ApplyPlaybackSnapshot()
{
	if (!HasActorBegunPlay())
	{
		return;
	}

	if (Pieces.IsEmpty())
	{
		RebuildPieceList();
	}

	switch (PlaybackSnapshot.Phase)
	{
	case EGPLevelBuildPlaybackPhase::Waiting:
		if (bWaitForColosseumPortalArrival)
		{
			ResetBuild(/*bMovePiecesToStart=*/true);
		}
		LastAppliedPlaybackRevision = PlaybackSnapshot.Revision;
		break;

	case EGPLevelBuildPlaybackPhase::Playing:
	{
		const float ElapsedSeconds = static_cast<float>(FMath::Max(
			0.0,
			GetSynchronizedWorldTimeSeconds() - PlaybackSnapshot.StartServerTime));
		const bool bNewPlaybackRevision =
			LastAppliedPlaybackRevision != PlaybackSnapshot.Revision;
		if (bNewPlaybackRevision || !bIsBuilding)
		{
			bUseSynchronizedBuildClock = true;
			StartBuildAtElapsedTime(
				ElapsedSeconds,
				/*bNotifyBuildStarted=*/bNewPlaybackRevision);
			bUseSynchronizedBuildClock =
				PlaybackSnapshot.Phase == EGPLevelBuildPlaybackPhase::Playing
					&& bIsBuilding;
		}
		LastAppliedPlaybackRevision = PlaybackSnapshot.Revision;
		break;
	}

	case EGPLevelBuildPlaybackPhase::Finished:
		FinishBuild();
		LastAppliedPlaybackRevision = PlaybackSnapshot.Revision;
		break;
	}
}

void AGP_LevelBuildAnimator::MarkBuildFinishedAuthoritative()
{
	if (!HasServerAuthority()
		|| PlaybackSnapshot.Phase != EGPLevelBuildPlaybackPhase::Playing)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AuthoritativeBuildTimerHandle);
	}

	PlaybackSnapshot.Phase = EGPLevelBuildPlaybackPhase::Finished;
	ForceNetUpdate();
	ApplyPlaybackSnapshot();
	OnBuildFinishedNative.Broadcast(this);
}

void AGP_LevelBuildAnimator::HandleAuthoritativeBuildTimerExpired()
{
	if (!HasServerAuthority()
		|| PlaybackSnapshot.Phase != EGPLevelBuildPlaybackPhase::Playing)
	{
		return;
	}

	FinishBuild();
}

double AGP_LevelBuildAnimator::GetSynchronizedWorldTimeSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GameState = World->GetGameState())
		{
			return GameState->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}
	return 0.0;
}

void AGP_LevelBuildAnimator::SortPieces()
{
	Pieces.Sort([](const FGPLevelBuildPiece& Left, const FGPLevelBuildPiece& Right)
	{
		if (Left.PriorityBand != Right.PriorityBand)
		{
			return Left.PriorityBand < Right.PriorityBand;
		}

		if (!FMath::IsNearlyEqual(Left.SortPrimary, Right.SortPrimary))
		{
			return Left.SortPrimary < Right.SortPrimary;
		}

		if (!FMath::IsNearlyEqual(Left.SortSecondary, Right.SortSecondary))
		{
			return Left.SortSecondary < Right.SortSecondary;
		}

		return Left.RandomValue < Right.RandomValue;
	});
}

void AGP_LevelBuildAnimator::BuildSortKeys()
{
	float MinRadius = TNumericLimits<float>::Max();
	float MaxRadius = 0.0f;

	for (const FGPLevelBuildPiece& Piece : Pieces)
	{
		MinRadius = FMath::Min(MinRadius, Piece.BoundsRadius);
		MaxRadius = FMath::Max(MaxRadius, Piece.BoundsRadius);
	}

	if (Pieces.Num() == 0 || MinRadius == TNumericLimits<float>::Max())
	{
		return;
	}

	const float RadiusRange = FMath::Max(KINDA_SMALL_NUMBER, MaxRadius - MinRadius);

	for (FGPLevelBuildPiece& Piece : Pieces)
	{
		Piece.SizeAlpha = FMath::Clamp((Piece.BoundsRadius - MinRadius) / RadiusRange, 0.0f, 1.0f);

		switch (Piece.EffectiveOrderMode)
		{
		case EGPLevelBuildOrderMode::HeightThenRandom:
			Piece.SortPrimary = Piece.HeightKey;
			Piece.SortSecondary = Piece.RandomValue;
			break;

		case EGPLevelBuildOrderMode::DistanceThenHeight:
			Piece.SortPrimary = Piece.DistanceKey;
			Piece.SortSecondary = Piece.HeightKey;
			break;

		case EGPLevelBuildOrderMode::Random:
			Piece.SortPrimary = Piece.RandomValue;
			Piece.SortSecondary = 0.0f;
			break;

		case EGPLevelBuildOrderMode::HeightThenDistance:
		default:
			Piece.SortPrimary = Piece.HeightKey;
			Piece.SortSecondary = Piece.DistanceKey;
			break;
		}
	}
}

void AGP_LevelBuildAnimator::PrepareStartTransforms()
{
	const FVector PivotLocation = ResolvePivotLocation();
	float CurrentStartTime = 0.0f;

	for (int32 Index = 0; Index < Pieces.Num(); ++Index)
	{
		FGPLevelBuildPiece& Piece = Pieces[Index];
		const FTransform& FinalTransform = Piece.FinalTransform;
		const FVector FinalLocation = FinalTransform.GetLocation();
		FVector FromPivot = FinalLocation - PivotLocation;
		FromPivot.Z = 0.0f;

		if (FromPivot.IsNearlyZero())
		{
			FromPivot = FVector::ForwardVector;
		}

		const FVector RadialDirection = FromPivot.GetSafeNormal();
		const FVector TangentDirection = FVector::CrossProduct(FVector::UpVector, RadialDirection).GetSafeNormal();
		const float SideSign = Piece.RandomValue < 0.5f ? -1.0f : 1.0f;
		const float SpiralAmount = MirrorSpiralRadius * (0.45f + Piece.RandomValue);

		FVector StartLocation = FinalLocation;
		StartLocation.Z -= UndergroundOffset;
		StartLocation += TangentDirection * SpiralAmount * SideSign;
		StartLocation += RadialDirection * SpiralAmount * 0.35f;

		const FRotator TwistRotation(
			StartPitchRollTwist * SideSign,
			StartYawTwist * SideSign * (0.65f + Piece.RandomValue),
			StartPitchRollTwist * (Piece.RandomValue - 0.5f) * 2.0f);

		FTransform StartTransform = FinalTransform;
		StartTransform.SetLocation(StartLocation);
		StartTransform.SetRotation((TwistRotation.Quaternion() * FinalTransform.GetRotation()).GetNormalized());
		StartTransform.SetScale3D(FinalTransform.GetScale3D() * FMath::Max(0.01f, StartScaleMultiplier));

		Piece.StartTransform = StartTransform;
		Piece.StartTime = CurrentStartTime;

		const float DurationMultiplier = SizeTimingMode == EGPLevelBuildSizeTimingMode::SmallPiecesFaster
			? ResolveSizeTimingMultiplier(Piece.SizeAlpha, SmallPieceDurationMultiplier, LargePieceDurationMultiplier)
			: 1.0f;
		const float Jitter = 1.0f + ((Piece.RandomValue - 0.5f) * 2.0f * DurationRandomJitterRatio);
		Piece.Duration = PieceDuration * DurationMultiplier * FMath::Max(0.05f, Jitter);

		const float DelayMultiplier = SizeTimingMode == EGPLevelBuildSizeTimingMode::SmallPiecesFaster
			? ResolveSizeTimingMultiplier(Piece.SizeAlpha, SmallPieceDelayMultiplier, LargePieceDelayMultiplier)
			: 1.0f;
		CurrentStartTime += PieceOverlapDelay * DelayMultiplier;
		Piece.bFinished = false;
	}
}

void AGP_LevelBuildAnimator::ApplyPieceTransform(FGPLevelBuildPiece& Piece, float LocalAlpha) const
{
	if (!IsValid(Piece.Actor))
	{
		return;
	}

	const float EasedAlpha = EaseAlpha(LocalAlpha);
	const FVector BaseLocation = FMath::Lerp(Piece.StartTransform.GetLocation(), Piece.FinalTransform.GetLocation(), EasedAlpha);
	const float OvershootWave = FMath::Sin(LocalAlpha * PI);
	FVector NewLocation = BaseLocation;
	NewLocation.Z += RiseOvershoot * OvershootWave * (1.0f - EasedAlpha);

	const FQuat NewRotation = FQuat::Slerp(Piece.StartTransform.GetRotation(), Piece.FinalTransform.GetRotation(), EasedAlpha).GetNormalized();
	const FVector FinalScale = Piece.FinalTransform.GetScale3D();
	const FVector StartScale = Piece.StartTransform.GetScale3D();
	const FVector OvershootScale = FinalScale * (1.0f + ScaleOvershootRatio * OvershootWave);
	const FVector NewScale = LocalAlpha < 0.82f
		? FMath::Lerp(StartScale, OvershootScale, EasedAlpha)
		: FMath::Lerp(OvershootScale, FinalScale, (LocalAlpha - 0.82f) / 0.18f);

	FRotator RotationOvershoot = FRotator::ZeroRotator;
	if (RotationOvershootDegrees > 0.0f && LocalAlpha > 0.55f && LocalAlpha < 1.0f)
	{
		const float RotationWave = FMath::Sin((LocalAlpha - 0.55f) / 0.45f * PI);
		const float SideSign = Piece.RandomValue < 0.5f ? -1.0f : 1.0f;
		RotationOvershoot = FRotator(0.0f, RotationOvershootDegrees * RotationWave * SideSign, 0.0f);
	}

	const FTransform NewTransform(RotationOvershoot.Quaternion() * NewRotation, NewLocation, NewScale);
	Piece.Actor->SetActorTransform(NewTransform, false, nullptr, ETeleportType::TeleportPhysics);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Piece.StartTransform.GetLocation(), Piece.FinalTransform.GetLocation(), FColor::Cyan, false, 0.0f, 0, 2.0f);
	}
}

float AGP_LevelBuildAnimator::EaseAlpha(float Alpha) const
{
	const float ClampedAlpha = FMath::Clamp(Alpha, 0.0f, 1.0f);
	return ClampedAlpha * ClampedAlpha * ClampedAlpha * (ClampedAlpha * (ClampedAlpha * 6.0f - 15.0f) + 10.0f);
}

float AGP_LevelBuildAnimator::CalculateTotalBuildTime() const
{
	float Result = 0.0f;
	for (const FGPLevelBuildPiece& Piece : Pieces)
	{
		Result = FMath::Max(Result, Piece.StartTime + Piece.Duration);
	}
	return Result;
}

FVector AGP_LevelBuildAnimator::ResolvePivotLocation() const
{
	return GetActorLocation();
}

EGPLevelBuildOrderMode AGP_LevelBuildAnimator::ResolveOrderModeForActor(const AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return OrderMode;
	}

	if (!HeightThenDistanceOrderTag.IsNone() && Candidate->ActorHasTag(HeightThenDistanceOrderTag))
	{
		return EGPLevelBuildOrderMode::HeightThenDistance;
	}

	if (!HeightThenRandomOrderTag.IsNone() && Candidate->ActorHasTag(HeightThenRandomOrderTag))
	{
		return EGPLevelBuildOrderMode::HeightThenRandom;
	}

	if (!DistanceThenHeightOrderTag.IsNone() && Candidate->ActorHasTag(DistanceThenHeightOrderTag))
	{
		return EGPLevelBuildOrderMode::DistanceThenHeight;
	}

	if (!RandomOrderTag.IsNone() && Candidate->ActorHasTag(RandomOrderTag))
	{
		return EGPLevelBuildOrderMode::Random;
	}

	return OrderMode;
}

int32 AGP_LevelBuildAnimator::ResolvePriorityBandForActor(const AActor* Candidate) const
{
	if (!IsValid(Candidate))
	{
		return 0;
	}

	if (!FirstPriorityTag.IsNone() && Candidate->ActorHasTag(FirstPriorityTag))
	{
		return -200;
	}

	if (!EarlyPriorityTag.IsNone() && Candidate->ActorHasTag(EarlyPriorityTag))
	{
		return -100;
	}

	if (!LatePriorityTag.IsNone() && Candidate->ActorHasTag(LatePriorityTag))
	{
		return 100;
	}

	if (!LastPriorityTag.IsNone() && Candidate->ActorHasTag(LastPriorityTag))
	{
		return 200;
	}

	return 0;
}

float AGP_LevelBuildAnimator::ResolveSizeTimingMultiplier(float SizeAlpha, float SmallMultiplier, float LargeMultiplier) const
{
	const float SmoothSizeAlpha = EaseAlpha(FMath::Clamp(SizeAlpha, 0.0f, 1.0f));
	return FMath::Lerp(SmallMultiplier, LargeMultiplier, SmoothSizeAlpha);
}
