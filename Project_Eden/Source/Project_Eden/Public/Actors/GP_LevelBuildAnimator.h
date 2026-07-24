#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_LevelBuildAnimator.generated.h"

class AGP_LevelBuildAnimator;

UENUM(BlueprintType)
enum class EGPLevelBuildOrderMode : uint8
{
	HeightThenDistance UMETA(DisplayName = "Height Then Distance"),
	HeightThenRandom UMETA(DisplayName = "Height Then Random"),
	DistanceThenHeight UMETA(DisplayName = "Distance Then Height"),
	Random UMETA(DisplayName = "Random")
};

UENUM(BlueprintType)
enum class EGPLevelBuildSizeTimingMode : uint8
{
	Constant UMETA(DisplayName = "Constant"),
	SmallPiecesFaster UMETA(DisplayName = "Small Pieces Faster")
};

UENUM(BlueprintType)
enum class EGPLevelBuildPlaybackPhase : uint8
{
	Waiting UMETA(DisplayName = "Waiting"),
	Playing UMETA(DisplayName = "Playing"),
	Finished UMETA(DisplayName = "Finished")
};

USTRUCT(BlueprintType)
struct FGPLevelBuildPlaybackSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	int32 Revision = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	EGPLevelBuildPlaybackPhase Phase = EGPLevelBuildPlaybackPhase::Waiting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	double StartServerTime = 0.0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float Duration = 0.0f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(
	FGPOnLevelBuildFinishedNative,
	AGP_LevelBuildAnimator*);

USTRUCT(BlueprintType)
struct FGPLevelBuildPiece
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	FTransform FinalTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	FTransform StartTransform = FTransform::Identity;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float StartTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float Duration = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float RandomValue = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float HeightKey = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float DistanceKey = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float BoundsRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float SizeAlpha = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float SortPrimary = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	float SortSecondary = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	int32 PriorityBand = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	EGPLevelBuildOrderMode EffectiveOrderMode = EGPLevelBuildOrderMode::HeightThenDistance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build")
	bool bFinished = false;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_LevelBuildAnimator : public AActor
{
	GENERATED_BODY()

public:
	AGP_LevelBuildAnimator();

	virtual void Tick(float DeltaSeconds) override;
	virtual void GetLifetimeReplicatedProps(
		TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Level Build")
	void RebuildPieceList();

	UFUNCTION(BlueprintCallable, Category = "Level Build")
	void StartBuild();

	UFUNCTION(BlueprintCallable, Category = "Level Build")
	void ResetBuild(bool bMovePiecesToStart = true);

	UFUNCTION(BlueprintCallable, Category = "Level Build")
	void FinishBuild();

	UFUNCTION(BlueprintPure, Category = "Level Build")
	bool IsBuilding() const { return bIsBuilding; }

	UFUNCTION(BlueprintPure, Category = "Level Build")
	float GetBuildAlpha() const;

	UFUNCTION(BlueprintPure, Category = "Level Build")
	float GetTotalBuildTime() const { return TotalBuildTime; }

	UFUNCTION(BlueprintPure, Category = "Level Build|Network")
	FName GetColosseumZoneId() const { return ColosseumZoneId; }

	UFUNCTION(BlueprintPure, Category = "Level Build|Network")
	EGPLevelBuildPlaybackPhase GetPlaybackPhase() const
	{
		return PlaybackSnapshot.Phase;
	}

	const FGPLevelBuildPlaybackSnapshot& GetPlaybackSnapshot() const
	{
		return PlaybackSnapshot;
	}

	// Authority-only entry point used after the server verifies the complete
	// party arrived through the Colosseum portal.
	bool StartBuildAuthoritative();

	FGPOnLevelBuildFinishedNative OnBuildFinishedNative;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Level Build")
	void BP_OnBuildStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Level Build")
	void BP_OnBuildPieceStarted(AActor* PieceActor, int32 PieceIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Level Build")
	void BP_OnBuildPieceFinished(AActor* PieceActor, int32 PieceIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Level Build")
	void BP_OnBuildFinished();

private:
	bool HasServerAuthority() const;
	bool ShouldControlActor(const AActor* Candidate) const;
	void StartBuildAtElapsedTime(
		float ElapsedSeconds,
		bool bNotifyBuildStarted);
	void ApplyPlaybackSnapshot();
	void MarkBuildFinishedAuthoritative();
	void HandleAuthoritativeBuildTimerExpired();
	double GetSynchronizedWorldTimeSeconds() const;
	void SortPieces();
	void BuildSortKeys();
	void PrepareStartTransforms();
	void ApplyPieceTransform(FGPLevelBuildPiece& Piece, float LocalAlpha) const;
	float EaseAlpha(float Alpha) const;
	float CalculateTotalBuildTime() const;
	FVector ResolvePivotLocation() const;
	EGPLevelBuildOrderMode ResolveOrderModeForActor(const AActor* Candidate) const;
	int32 ResolvePriorityBandForActor(const AActor* Candidate) const;
	float ResolveSizeTimingMultiplier(float SizeAlpha, float SmallMultiplier, float LargeMultiplier) const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|01 Target", meta = (AllowPrivateAccess = "true", ToolTip = "Actors with this tag are controlled by the build animation. Add this tag to PLAZA_DE_TOROS structure actors in the level."))
	FName TargetActorTag = TEXT("PLAZA_DE_TOROS");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|01 Target", meta = (AllowPrivateAccess = "true", ToolTip = "Optional explicit Colosseum ZoneId. Leave None when this is the only Colosseum animator in the world."))
	FName ColosseumZoneId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|01 Target", meta = (AllowPrivateAccess = "true", ToolTip = "If false, actors hidden in editor/game are ignored when building the piece list."))
	bool bIncludeHiddenActors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|01 Target", AdvancedDisplay, meta = (AllowPrivateAccess = "true", ToolTip = "If false, actors attached to this animator are ignored. Useful to avoid accidentally animating helper children."))
	bool bIncludeSelfAttachedActors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|02 Playback", meta = (AllowPrivateAccess = "true", ToolTip = "Starts the construction automatically on BeginPlay. Disable this if another Blueprint should call StartBuild manually."))
	bool bAutoStartOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|02 Playback", meta = (AllowPrivateAccess = "true", ToolTip = "Keeps this map-placed animator reset until the authoritative Colosseum portal arrival starts its replicated playback."))
	bool bWaitForColosseumPortalArrival = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering", meta = (AllowPrivateAccess = "true", ToolTip = "Controls which piece starts first. Height modes build from low Z to high Z."))
	EGPLevelBuildOrderMode OrderMode = EGPLevelBuildOrderMode::HeightThenDistance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actor tag override. Actors with this tag use Height Then Distance ordering instead of the default order mode."))
	FName HeightThenDistanceOrderTag = TEXT("BuildOrder.HeightThenDistance");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actor tag override. Actors with this tag use Height Then Random ordering instead of the default order mode."))
	FName HeightThenRandomOrderTag = TEXT("BuildOrder.HeightThenRandom");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actor tag override. Actors with this tag use Distance Then Height ordering instead of the default order mode."))
	FName DistanceThenHeightOrderTag = TEXT("BuildOrder.DistanceThenHeight");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actor tag override. Actors with this tag use Random ordering instead of the default order mode."))
	FName RandomOrderTag = TEXT("BuildOrder.Random");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Priority Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actors with this tag are built before normal pieces, regardless of ordering mode."))
	FName FirstPriorityTag = TEXT("BuildPriority.First");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Priority Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actors with this tag are built before normal pieces but after First priority pieces."))
	FName EarlyPriorityTag = TEXT("BuildPriority.Early");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Priority Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actors with this tag are built after normal pieces but before Last priority pieces."))
	FName LatePriorityTag = TEXT("BuildPriority.Late");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Priority Tags", meta = (AllowPrivateAccess = "true", ToolTip = "Actors with this tag are built at the end, regardless of ordering mode."))
	FName LastPriorityTag = TEXT("BuildPriority.Last");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s", ToolTip = "How long each individual structure piece takes to reach its final transform."))
	float PieceDuration = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s", ToolTip = "Delay between each piece start. Lower values make more pieces build together."))
	float PieceOverlapDelay = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ToolTip = "Controls whether small pieces build faster and closer together than large pieces. Recommended for railings and dense detail parts."))
	EGPLevelBuildSizeTimingMode SizeTimingMode = EGPLevelBuildSizeTimingMode::SmallPiecesFaster;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.05", ClampMax = "2.0", ToolTip = "Duration multiplier for the smallest pieces when Size Timing Mode is Small Pieces Faster."))
	float SmallPieceDurationMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.05", ClampMax = "2.0", ToolTip = "Duration multiplier for the largest pieces when Size Timing Mode is Small Pieces Faster."))
	float LargePieceDurationMultiplier = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "2.0", ToolTip = "Start-delay multiplier for the smallest pieces. Lower values make dense small parts build in tighter bursts."))
	float SmallPieceDelayMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "2.0", ToolTip = "Start-delay multiplier for the largest pieces."))
	float LargePieceDelayMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|04 Timing", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Random per-piece duration variation. 0.2 means each piece can be up to 20 percent faster or slower."))
	float DurationRandomJitterRatio = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm", ToolTip = "Z values within this distance are treated as the same height band before distance/random tie-breaking."))
	float SameHeightTolerance = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|03 Ordering", AdvancedDisplay, meta = (AllowPrivateAccess = "true", ToolTip = "Seed used by random ordering and random start offsets. Same seed gives repeatable construction."))
	int32 RandomSeed = 137;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|05 Motion", meta = (AllowPrivateAccess = "true", Units = "cm", ToolTip = "How far below the final transform each piece starts."))
	float UndergroundOffset = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|06 Mirror Dimension", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm", ToolTip = "Sideways spiral offset from the final location. Higher values feel more warped and mirror-dimension-like."))
	float MirrorSpiralRadius = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|06 Mirror Dimension", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "deg", ToolTip = "Initial yaw twist before the piece rotates back into its final orientation."))
	float StartYawTwist = 130.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|06 Mirror Dimension", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "deg", ToolTip = "Initial pitch/roll twist before the piece rotates back into its final orientation."))
	float StartPitchRollTwist = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|05 Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", ToolTip = "Initial scale multiplier. 0.08 means pieces start at 8 percent of final scale."))
	float StartScaleMultiplier = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|05 Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm", ToolTip = "Small vertical overshoot during rise. Increase for heavier magical pop-up motion."))
	float RiseOvershoot = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|06 Mirror Dimension", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "deg", ToolTip = "Extra late rotation wobble before settling into the final transform."))
	float RotationOvershootDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|05 Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0", ToolTip = "Temporary scale overshoot ratio. 0.08 means up to 8 percent larger before settling."))
	float ScaleOvershootRatio = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Level Build|99 Debug", meta = (AllowPrivateAccess = "true", ToolTip = "Draws each piece start-to-final path while building."))
	bool bDrawDebug = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build", meta = (AllowPrivateAccess = "true"))
	TArray<FGPLevelBuildPiece> Pieces;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build", meta = (AllowPrivateAccess = "true"))
	bool bIsBuilding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build", meta = (AllowPrivateAccess = "true"))
	float BuildTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Level Build", meta = (AllowPrivateAccess = "true"))
	float TotalBuildTime = 0.0f;

	UPROPERTY(ReplicatedUsing = OnRep_PlaybackSnapshot, VisibleAnywhere, BlueprintReadOnly, Category = "Level Build|Network", meta = (AllowPrivateAccess = "true"))
	FGPLevelBuildPlaybackSnapshot PlaybackSnapshot;

	UFUNCTION()
	void OnRep_PlaybackSnapshot();

	int32 LastAppliedPlaybackRevision = INDEX_NONE;
	bool bUseSynchronizedBuildClock = false;
	FTimerHandle AuthoritativeBuildTimerHandle;
};
