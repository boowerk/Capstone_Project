#include "AI/Controllers/EnemyAIController.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "TimerManager.h"
#include "UObject/UObjectGlobals.h"

namespace
{
	float SanitizeFiniteFloat(float InValue, float FallbackValue, float MinValue, float MaxValue)
	{
		return FMath::IsFinite(InValue)
			? FMath::Clamp(InValue, MinValue, MaxValue)
			: FMath::Clamp(FallbackValue, MinValue, MaxValue);
	}

	FEnemyLLMEvaluation SanitizeEnemyEvaluationForRuntime(
		const FEnemyLLMEvaluation& InEvaluation,
		const FEnemyLLMEvaluation& FallbackEvaluation)
	{
		FEnemyLLMEvaluation SafeEvaluation = InEvaluation;
		SafeEvaluation.Aggression = SanitizeFiniteFloat(SafeEvaluation.Aggression, FallbackEvaluation.Aggression, 0.0f, 1.0f);
		SafeEvaluation.PreferredRange = SanitizeFiniteFloat(SafeEvaluation.PreferredRange, FallbackEvaluation.PreferredRange, 0.0f, 3000.0f);
		SafeEvaluation.RetreatThreshold = SanitizeFiniteFloat(SafeEvaluation.RetreatThreshold, FallbackEvaluation.RetreatThreshold, 0.0f, 1.0f);
		SafeEvaluation.ChasePersistence = SanitizeFiniteFloat(SafeEvaluation.ChasePersistence, FallbackEvaluation.ChasePersistence, 0.0f, 1.0f);
		SafeEvaluation.CoverPreference = SanitizeFiniteFloat(SafeEvaluation.CoverPreference, FallbackEvaluation.CoverPreference, 0.0f, 1.0f);
		SafeEvaluation.ValidateAndClamp();
		return SafeEvaluation;
	}

	bool AreEnemyEvaluationsEffectivelyEqual(const FEnemyLLMEvaluation& Left, const FEnemyLLMEvaluation& Right)
	{
		return Left.EnemyMode == Right.EnemyMode
			&& Left.FocusTargetRule == Right.FocusTargetRule
			&& FMath::IsNearlyEqual(Left.Aggression, Right.Aggression, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.PreferredRange, Right.PreferredRange, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.RetreatThreshold, Right.RetreatThreshold, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.ChasePersistence, Right.ChasePersistence, KINDA_SMALL_NUMBER)
			&& FMath::IsNearlyEqual(Left.CoverPreference, Right.CoverPreference, KINDA_SMALL_NUMBER);
	}

	const TCHAR* GetSharedBehaviorTreeFallbackPath()
	{
		return TEXT("/Game/Characters/EnemyCharacter/BT/Common/BT_EnemyCommon.BT_EnemyCommon");
	}

	const TCHAR* GetSharedBlackboardFallbackPath()
	{
		return TEXT("/Game/Characters/EnemyCharacter/BT/Common/BB_EnemyCommon.BB_EnemyCommon");
	}

	FEnemyLLMEvaluation BuildBossRuntimeEvaluationTestSample(int32 SampleIndex)
	{
		FEnemyLLMEvaluation Evaluation = FEnemyLLMEvaluation::MakeSafeDefault();

		switch (SampleIndex % 4)
		{
		case 0:
			// Basic sample: close/defensive values boost the boss basic scratch above sweep.
			Evaluation.EnemyMode = EEnemyMode::Hold;
			Evaluation.Aggression = 0.2f;
			Evaluation.PreferredRange = 650.0f;
			Evaluation.RetreatThreshold = 0.0f;
			Evaluation.ChasePersistence = 0.4f;
			Evaluation.CoverPreference = 0.65f;
			Evaluation.FocusTargetRule = EEnemyFocusTargetRule::Weakest;
			break;

		case 1:
			// Sweep sample: aggressive pressure at preferred range should select the fan attack.
			Evaluation.EnemyMode = EEnemyMode::Pressure;
			Evaluation.Aggression = 0.95f;
			Evaluation.PreferredRange = 600.0f;
			Evaluation.RetreatThreshold = 0.0f;
			Evaluation.ChasePersistence = 0.9f;
			Evaluation.CoverPreference = 0.1f;
			Evaluation.FocusTargetRule = EEnemyFocusTargetRule::PlayerFirst;
			break;

		case 2:
			// Area sample: very short preferred range makes the current player distance score as far pressure.
			Evaluation.EnemyMode = EEnemyMode::Hold;
			Evaluation.Aggression = 0.55f;
			Evaluation.PreferredRange = 120.0f;
			Evaluation.RetreatThreshold = 0.0f;
			Evaluation.ChasePersistence = 0.35f;
			Evaluation.CoverPreference = 0.75f;
			Evaluation.FocusTargetRule = EEnemyFocusTargetRule::CurrentThreat;
			break;

		default:
			// Summon sample: retreat mode opens add pressure in the boss tactics service.
			Evaluation.EnemyMode = EEnemyMode::Retreat;
			Evaluation.Aggression = 0.15f;
			Evaluation.PreferredRange = 120.0f;
			Evaluation.RetreatThreshold = 0.0f;
			Evaluation.ChasePersistence = 0.25f;
			Evaluation.CoverPreference = 0.8f;
			Evaluation.FocusTargetRule = EEnemyFocusTargetRule::CurrentThreat;
			break;
		}

		Evaluation.ValidateAndClamp();
		return Evaluation;
	}

	AActor* SelectNearestTargetCandidate(const APawn* ControlledPawn, const TArray<AActor*>& Candidates)
	{
		if (!IsValid(ControlledPawn))
		{
			return nullptr;
		}

		AActor* BestTargetActor = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : Candidates)
		{
			const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), CandidateActor->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestTargetActor = CandidateActor;
			}
		}

		return BestTargetActor;
	}

	AActor* SelectPlayerFirstTargetCandidate(const APawn* ControlledPawn, const TArray<AActor*>& Candidates)
	{
		if (!IsValid(ControlledPawn))
		{
			return nullptr;
		}

		AActor* BestPlayerActor = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : Candidates)
		{
			const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
			if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(ControlledPawn->GetActorLocation(), CandidateActor->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestPlayerActor = CandidateActor;
			}
		}

		return BestPlayerActor;
	}
}

AEnemyAIController::AEnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	SightSenseConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightSenseConfig"));
	HearingSenseConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingSenseConfig"));

	// Keep sensing on the engine perception component and keep this controller as the target selector.
	SetPerceptionComponent(*EnemyPerceptionComponent);

	TargetActorClass = APawn::StaticClass();

	if (IsValid(EnemyPerceptionComponent))
	{
		EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	ConfigureSightSense();
	ConfigureHearingSense();
}

FPathFollowingRequestResult AEnemyAIController::MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath)
{
	if (ShouldUseDirectFlyingMove(GetPawn(), MoveRequest))
	{
		FAIMoveRequest DirectFlyingRequest(ResolveDirectFlyingGoal(GetPawn(), MoveRequest));
		DirectFlyingRequest
			.SetUsePathfinding(false)
			.SetAllowPartialPath(false)
			.SetRequireNavigableEndLocation(false)
			.SetProjectGoalLocation(false)
			.SetCanStrafe(MoveRequest.CanStrafe())
			.SetReachTestIncludesAgentRadius(MoveRequest.IsReachTestIncludingAgentRadius())
			.SetReachTestIncludesGoalRadius(MoveRequest.IsReachTestIncludingGoalRadius())
			.SetAcceptanceRadius(MoveRequest.GetAcceptanceRadius())
			.SetUserData(MoveRequest.GetUserData())
			.SetUserFlags(MoveRequest.GetUserFlags());

		// Abstract navigation supplies a straight path so flying patrol does not fail every BT iteration on ground NavMesh projection.
		return Super::MoveTo(DirectFlyingRequest, OutPath);
	}

	return Super::MoveTo(MoveRequest, OutPath);
}

bool AEnemyAIController::ShouldUseDirectFlyingMove(const APawn* ControlledPawn, const FAIMoveRequest& MoveRequest)
{
	const ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn);
	const UCharacterMovementComponent* MovementComponent = IsValid(ControlledCharacter)
		? ControlledCharacter->GetCharacterMovement()
		: nullptr;
	return !MoveRequest.IsMoveToActorRequest()
		&& IsValid(MovementComponent)
		&& MovementComponent->MovementMode == MOVE_Flying;
}

FVector AEnemyAIController::ResolveDirectFlyingGoal(const APawn* ControlledPawn, const FAIMoveRequest& MoveRequest)
{
	FVector DirectGoal = MoveRequest.GetGoalLocation();
	if (IsValid(ControlledPawn))
	{
		// Recast patrol/EQS points are ground-based; preserve the flying pawn's current altitude for direct movement.
		DirectGoal.Z = ControlledPawn->GetActorLocation().Z;
	}
	return DirectGoal;
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogEnemyAI, Log, TEXT("[AI] Possess: Controller=%s Pawn=%s"), *GetName(), *EnemyAIDebugUtils::DescribeActor(InPawn));

	ConfigureSightSense();
	ConfigureHearingSense();
	InitializeBehaviorTree(InPawn);
	StartEvaluationRefreshLoop();

	if (bHasPendingEnemyEvaluation)
	{
		HandlePendingEnemyEvaluationTimerElapsed();
	}

	RequestTargetActorReevaluation();
}

void AEnemyAIController::OnUnPossess()
{
	UE_LOG(LogEnemyAI, Log, TEXT("[AI] UnPossess: Controller=%s Pawn=%s"), *GetName(), *EnemyAIDebugUtils::DescribeActor(GetPawn()));

	StopEvaluationRefreshLoop();
	GetWorldTimerManager().ClearTimer(PendingEnemyEvaluationTimerHandle);
	bLeashReturnHomeActive = false;
	bHasPendingBehaviorTreeRootReevaluation = false;
	PendingBehaviorTreeRootReevaluationRetryCount = 0;
	BossRuntimeEvaluationTestIndex = 0;

	// TargetActor belongs to perception selection, so clear it when the controller releases the pawn.
	SetBlackboardTargetActor(nullptr);

	Super::OnUnPossess();
}

bool AEnemyAIController::SubmitEnemyEvaluation(const FEnemyLLMEvaluation& InEvaluation, bool bForceImmediate)
{
	const FEnemyLLMEvaluation FallbackEvaluation = bHasLastAppliedEnemyEvaluation
		? LastAppliedEnemyEvaluation
		: FEnemyLLMEvaluation::MakeSafeDefault();

	const FEnemyLLMEvaluation SafeEvaluation = SanitizeEnemyEvaluationForRuntime(InEvaluation, FallbackEvaluation);
	if (bHasLastAppliedEnemyEvaluation && AreEnemyEvaluationsEffectivelyEqual(LastAppliedEnemyEvaluation, SafeEvaluation))
	{
		UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] Ignored duplicate evaluation: %s"), *EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));
		return true;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World != nullptr ? World->GetTimeSeconds() : 0.0f;
	const float TimeSinceLastApply = CurrentTime - LastEnemyEvaluationApplyTime;
	const bool bCanApplyNow = bForceImmediate
		|| !bHasLastAppliedEnemyEvaluation
		|| TimeSinceLastApply >= MinEnemyEvaluationApplyInterval;

	if (bCanApplyNow)
	{
		GetWorldTimerManager().ClearTimer(PendingEnemyEvaluationTimerHandle);
		bHasPendingEnemyEvaluation = false;
		return ApplyEnemyEvaluationInternal(SafeEvaluation);
	}

	// Keep only the newest pending evaluation so rapid LLM updates do not thrash Blackboard observers.
	PendingEnemyEvaluation = SafeEvaluation;
	bHasPendingEnemyEvaluation = true;

	const float RemainingDelay = FMath::Max(0.0f, MinEnemyEvaluationApplyInterval - TimeSinceLastApply);
	UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] Delayed apply %.2fs: %s"), RemainingDelay, *EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));

	GetWorldTimerManager().SetTimer(
		PendingEnemyEvaluationTimerHandle,
		this,
		&ThisClass::HandlePendingEnemyEvaluationTimerElapsed,
		RemainingDelay,
		false);

	return true;
}

bool AEnemyAIController::SubmitEnemyEvaluationFromJson(const FString& JsonPayload, bool bForceImmediate)
{
	FEnemyLLMEvaluation ParsedEvaluation;
	const bool bParsed = FEnemyLLMEvaluationParser::ParseFromJson(JsonPayload, ParsedEvaluation);
	if (!bParsed)
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[Eval] Failed to apply JSON evaluation: Controller=%s"), *GetName());
		return false;
	}

	return SubmitEnemyEvaluation(ParsedEvaluation, bForceImmediate);
}

bool AEnemyAIController::IsBossRuntimeEvaluationTestCycleActive() const
{
	return ShouldUseBossRuntimeEvaluationTestCycle();
}

bool AEnemyAIController::ApplyEnemyEvaluationToBlackboard(const FEnemyLLMEvaluation& InEvaluation)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[Blackboard] Failed to apply evaluation - missing Blackboard: %s"), *GetName());
		return false;
	}

	FEnemyLLMEvaluation SafeEvaluation = InEvaluation;
	SafeEvaluation.ValidateAndClamp();

	const bool bModeChanged = !bHasLastAppliedEnemyEvaluation || LastAppliedEnemyEvaluation.EnemyMode != SafeEvaluation.EnemyMode;
	if (bModeChanged)
	{
		const FString PreviousMode = bHasLastAppliedEnemyEvaluation
			? LastAppliedEnemyEvaluation.GetEnemyModeBlackboardValue().ToString()
			: TEXT("None");

		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[Mode] %s -> %s (%s)"),
			*PreviousMode,
			*SafeEvaluation.GetEnemyModeBlackboardValue().ToString(),
			*EnemyAIDebugUtils::DescribeActor(GetPawn()));
	}

	// Controller writes only high-level tuning values; BT service owns tactical branch keys.
	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::EnemyMode, SafeEvaluation.GetEnemyModeBlackboardValue());
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::Aggression, SafeEvaluation.Aggression);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::PreferredRange, SafeEvaluation.PreferredRange);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::RetreatThreshold, SafeEvaluation.RetreatThreshold);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::ChasePersistence, SafeEvaluation.ChasePersistence);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::CoverPreference, SafeEvaluation.CoverPreference);
	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::FocusTargetRule, SafeEvaluation.GetFocusTargetRuleBlackboardValue());

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[Blackboard] Applied tuning: Pawn=%s %s"),
		*EnemyAIDebugUtils::DescribeActor(GetPawn()),
		*EnemyAIDebugUtils::DescribeEvaluation(SafeEvaluation));

	// Tuning or focus-rule changes may alter the best perceived target, so re-score current candidates.
	RequestTargetActorReevaluation();
	return true;
}

bool AEnemyAIController::InitializeBehaviorTree(APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Behavior Tree initialization failed - invalid pawn."));
		return false;
	}

	UBehaviorTree* BehaviorTreeAssetToUse = nullptr;
	UBlackboardData* BlackboardAssetToUse = nullptr;
	ResolveBehaviorAssets(InPawn, BehaviorTreeAssetToUse, BlackboardAssetToUse);

	if (!IsValid(BehaviorTreeAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Missing Behavior Tree asset: %s"), *GetNameSafe(InPawn));
		return false;
	}

	if (!IsValid(BlackboardAssetToUse))
	{
		BlackboardAssetToUse = BehaviorTreeAssetToUse->BlackboardAsset;
	}

	if (!IsValid(BlackboardAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Missing Blackboard asset: %s"), *GetNameSafe(BehaviorTreeAssetToUse));
		return false;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!UseBlackboard(BlackboardAssetToUse, BlackboardComponent) || !IsValid(BlackboardComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Blackboard initialization failed: %s"), *GetNameSafe(InPawn));
		return false;
	}

	ValidateSharedBlackboardSchema(BlackboardComponent);
	InitializeBlackboardValues(InPawn);

	if (!RunBehaviorTree(BehaviorTreeAssetToUse))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[AI] Behavior Tree run failed: %s"), *GetNameSafe(InPawn));
		return false;
	}

	UE_LOG(LogEnemyAI, Log, TEXT("[AI] Behavior Tree ready: BT=%s BB=%s"), *GetNameSafe(BehaviorTreeAssetToUse), *GetNameSafe(BlackboardAssetToUse));
	return true;
}

void AEnemyAIController::InitializeBlackboardValues(APawn* InPawn)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent) || !IsValid(InPawn))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[Blackboard] Skipped initial values: Controller=%s Pawn=%s"), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	// Initial non-tactical values are safe here; live tactical keys are written by BTS_UpdateEnemyTactics.
	BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	const AGP_EnemyCharacter* EnemyPawnCharacter = Cast<AGP_EnemyCharacter>(InPawn);
	const FVector HomeLocation = IsValid(EnemyPawnCharacter) ? EnemyPawnCharacter->GetBehaviorAnchorLocation() : InPawn->GetActorLocation();
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::HomeLocation, HomeLocation);
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, HomeLocation);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceFromHome, 0.0f);
	BlackboardComponent->SetValueAsBool(EnemyBlackboardKeys::bShouldReturnHome, false);

	FEnemyLLMEvaluation InitialEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
	{
		EnemyCharacter->BuildInitialEnemyEvaluation(InitialEvaluation);
	}

	ApplyEnemyEvaluationToBlackboard(InitialEvaluation);
	LastAppliedEnemyEvaluation = InitialEvaluation;
	bHasLastAppliedEnemyEvaluation = true;
	LastEnemyEvaluationApplyTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
}

UBlackboardComponent* AEnemyAIController::GetEnemyBlackboardComponent()
{
	return GetBlackboardComponent();
}

bool AEnemyAIController::ApplyEnemyEvaluationInternal(const FEnemyLLMEvaluation& InEvaluation)
{
	if (!ApplyEnemyEvaluationToBlackboard(InEvaluation))
	{
		// If Blackboard is not ready yet, keep the latest tuning and apply it after possession.
		PendingEnemyEvaluation = InEvaluation;
		bHasPendingEnemyEvaluation = true;
		UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] Queued evaluation until Blackboard is ready: %s"), *EnemyAIDebugUtils::DescribeEvaluation(InEvaluation));
		return false;
	}

	LastAppliedEnemyEvaluation = InEvaluation;
	bHasLastAppliedEnemyEvaluation = true;
	bHasPendingEnemyEvaluation = false;
	LastEnemyEvaluationApplyTime = GetWorld() != nullptr ? GetWorld()->GetTimeSeconds() : 0.0f;
	return true;
}

void AEnemyAIController::HandlePendingEnemyEvaluationTimerElapsed()
{
	if (!bHasPendingEnemyEvaluation)
	{
		return;
	}

	const FEnemyLLMEvaluation QueuedEvaluation = PendingEnemyEvaluation;
	bHasPendingEnemyEvaluation = false;

	UE_LOG(LogEnemyAI, Verbose, TEXT("[Eval] Applying delayed evaluation: %s"), *EnemyAIDebugUtils::DescribeEvaluation(QueuedEvaluation));
	ApplyEnemyEvaluationInternal(QueuedEvaluation);
}

void AEnemyAIController::HandleEvaluationRefreshTimerElapsed()
{
	if (ShouldUseBossRuntimeEvaluationTestCycle())
	{
		const int32 SampleIndex = BossRuntimeEvaluationTestIndex++;
		const FEnemyLLMEvaluation Evaluation = BuildBossRuntimeEvaluationTestSample(SampleIndex);
		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[EvalTest] Boss runtime evaluation sample %d: %s"),
			SampleIndex % 4,
			*EnemyAIDebugUtils::DescribeEvaluation(Evaluation));
		SubmitEnemyEvaluation(Evaluation, true);
		return;
	}

	// Future hook: request an async LLM update, then call SubmitEnemyEvaluation with the response.
}

void AEnemyAIController::StartEvaluationRefreshLoop()
{
	const bool bUseBossTestCycle = ShouldUseBossRuntimeEvaluationTestCycle();
	const float EffectiveRefreshInterval = bUseBossTestCycle
		? BossRuntimeEvaluationTestInterval
		: EvaluationRefreshInterval;

	if ((!bEnableEvaluationRefreshLoop && !bUseBossTestCycle) || EffectiveRefreshInterval <= 0.0f)
	{
		return;
	}

	// Keep refreshes coarse-grained; Behavior Tree services handle moment-to-moment tactics.
	GetWorldTimerManager().SetTimer(
		EvaluationRefreshTimerHandle,
		this,
		&ThisClass::HandleEvaluationRefreshTimerElapsed,
		EffectiveRefreshInterval,
		true);

	if (bUseBossTestCycle)
	{
		// Apply the first sample immediately so PIE logs show pattern rotation without waiting for the first timer tick.
		HandleEvaluationRefreshTimerElapsed();
	}
}

void AEnemyAIController::StopEvaluationRefreshLoop()
{
	if (GetWorld() != nullptr)
	{
		GetWorldTimerManager().ClearTimer(EvaluationRefreshTimerHandle);
	}
}

bool AEnemyAIController::ShouldUseBossRuntimeEvaluationTestCycle() const
{
	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetPawn());
	return bEnableBossRuntimeEvaluationTestCycle
		&& IsValid(EnemyCharacter)
		&& EnemyCharacter->IsBossEnemy();
}

void AEnemyAIController::ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const
{
	OutBehaviorTreeAsset = nullptr;
	OutBlackboardAsset = nullptr;

	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
	{
		// Pawn overrides must win over controller defaults so boss Blueprints can select their own BT/Blackboard.
		OutBehaviorTreeAsset = EnemyCharacter->GetBehaviorTreeAssetOverride();
		OutBlackboardAsset = EnemyCharacter->GetBlackboardAssetOverride();
	}

	if (!IsValid(OutBehaviorTreeAsset))
	{
		OutBehaviorTreeAsset = DefaultBehaviorTreeAsset;
	}

	if (!IsValid(OutBlackboardAsset))
	{
		OutBlackboardAsset = DefaultBlackboardAsset;
	}

	if (!IsValid(OutBehaviorTreeAsset) && bUseSharedBehaviorAssetFallback)
	{
		// Development fallback only; production enemies should reference assets explicitly in editor data.
		OutBehaviorTreeAsset = LoadObject<UBehaviorTree>(nullptr, GetSharedBehaviorTreeFallbackPath());
		if (IsValid(OutBehaviorTreeAsset))
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[AI] Loaded fallback BT: %s"), *GetNameSafe(OutBehaviorTreeAsset));
		}
	}

	if (!IsValid(OutBlackboardAsset) && IsValid(OutBehaviorTreeAsset) && IsValid(OutBehaviorTreeAsset->BlackboardAsset))
	{
		OutBlackboardAsset = OutBehaviorTreeAsset->BlackboardAsset;
	}

	if (!IsValid(OutBlackboardAsset) && bUseSharedBehaviorAssetFallback)
	{
		// Development fallback only; this keeps test maps usable while editor references are being wired.
		OutBlackboardAsset = LoadObject<UBlackboardData>(nullptr, GetSharedBlackboardFallbackPath());
		if (IsValid(OutBlackboardAsset))
		{
			UE_LOG(LogEnemyAI, Log, TEXT("[AI] Loaded fallback Blackboard: %s"), *GetNameSafe(OutBlackboardAsset));
		}
	}
}

bool AEnemyAIController::ValidateSharedBlackboardSchema(UBlackboardComponent* BlackboardComponent)
{
	if (!IsValid(BlackboardComponent))
	{
		return false;
	}

	static const FName RequiredKeys[] =
	{
		EnemyBlackboardKeys::TargetActor,
		EnemyBlackboardKeys::EnemyMode,
		EnemyBlackboardKeys::Aggression,
		EnemyBlackboardKeys::PreferredRange,
		EnemyBlackboardKeys::RetreatThreshold,
		EnemyBlackboardKeys::ChasePersistence,
		EnemyBlackboardKeys::CoverPreference,
		EnemyBlackboardKeys::HomeLocation,
		EnemyBlackboardKeys::MoveToLocation,
		EnemyBlackboardKeys::FocusTargetRule,
		EnemyBlackboardKeys::bShouldRetreat,
		EnemyBlackboardKeys::bCanAttack,
		EnemyBlackboardKeys::bShouldReposition,
		EnemyBlackboardKeys::bShouldChase,
		EnemyBlackboardKeys::bShouldReturnHome,
		EnemyBlackboardKeys::bHasLineOfSight,
		EnemyBlackboardKeys::DistanceToTarget,
		EnemyBlackboardKeys::DistanceFromHome,
		EnemyBlackboardKeys::HealthRatio,
	};

	TArray<FString> MissingKeys;
	for (const FName& KeyName : RequiredKeys)
	{
		if (BlackboardComponent->GetKeyID(KeyName) == FBlackboard::InvalidKey)
		{
			MissingKeys.Add(KeyName.ToString());
		}
	}

	if (MissingKeys.IsEmpty())
	{
		return true;
	}

	if (!bHasWarnedAboutMissingBlackboardKeys)
	{
		UE_LOG(
			LogEnemyAI,
			Warning,
			TEXT("[Blackboard] BB_EnemyCommon is missing required keys: %s"),
			*FString::Join(MissingKeys, TEXT(", ")));

		bHasWarnedAboutMissingBlackboardKeys = true;
	}

	return false;
}

void AEnemyAIController::ConfigureSightSense()
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(SightSenseConfig))
	{
		return;
	}

	float EffectiveSightRadius = SightRadius;
	float EffectiveLoseSightRadius = LoseSightRadius;
	float EffectivePeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetPawn()))
	{
		// Placed enemy settings win over controller defaults so designers can tune each enemy in the level.
		EffectiveSightRadius = EnemyCharacter->GetSightRadius();
		EffectiveLoseSightRadius = EnemyCharacter->GetLoseSightRadius();
		EffectivePeripheralVisionAngleDegrees = EnemyCharacter->GetPeripheralVisionAngleDegrees();
	}

	SightSenseConfig->SightRadius = EffectiveSightRadius;
	SightSenseConfig->LoseSightRadius = FMath::Max(EffectiveLoseSightRadius, EffectiveSightRadius);
	SightSenseConfig->PeripheralVisionAngleDegrees = EffectivePeripheralVisionAngleDegrees;
	SightSenseConfig->SetMaxAge(SightMaxAge);
	SightSenseConfig->AutoSuccessRangeFromLastSeenLocation = AutoSuccessRangeFromLastSeenLocation;
	SightSenseConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	SightSenseConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	SightSenseConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;

	// Configure the engine sense in one place so editor-exposed values stay authoritative.
	EnemyPerceptionComponent->ConfigureSense(*SightSenseConfig);
	EnemyPerceptionComponent->SetDominantSense(SightSenseConfig->GetSenseImplementation());
	EnemyPerceptionComponent->RequestStimuliListenerUpdate();
}

void AEnemyAIController::ConfigureHearingSense()
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(HearingSenseConfig))
	{
		return;
	}

	float EffectiveHearingRange = HearingRange;
	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetPawn()))
	{
		// The possessed enemy remains the single designer-facing source for its perception ranges.
		EffectiveHearingRange = EnemyCharacter->GetHearingRange();
	}

	HearingSenseConfig->HearingRange = FMath::Max(0.0f, EffectiveHearingRange);
	HearingSenseConfig->SetMaxAge(HearingMaxAge);
	HearingSenseConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	HearingSenseConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	HearingSenseConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;

	// Sight remains dominant for location resolution, while hearing can still acquire and remember candidates.
	EnemyPerceptionComponent->ConfigureSense(*HearingSenseConfig);
	EnemyPerceptionComponent->RequestStimuliListenerUpdate();
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[Perception] %s Target=%s %s"),
		*GetName(),
		*EnemyAIDebugUtils::DescribeActor(Actor),
		*EnemyAIDebugUtils::DescribeStimulus(Stimulus));

	RequestTargetActorReevaluation();
}

void AEnemyAIController::RequestTargetActorReevaluation()
{
	// Both perception events and tuning changes use this path to re-score visible or remembered sight candidates.
	RefreshTargetActorFromPerception();
}

void AEnemyAIController::RefreshTargetActorFromPerception()
{
	if (bLeashReturnHomeActive)
	{
		// The tactics service owns re-engagement using anchor-distance hysteresis; perception only records candidates here.
		SetBlackboardTargetActor(nullptr);
		return;
	}

	SetBlackboardTargetActor(SelectBestTargetActorFromPerception());
}

bool AEnemyAIController::HasCurrentlyPerceivedTargetCandidate() const
{
	TArray<AActor*> CurrentlyPerceivedCandidates;
	TArray<AActor*> KnownCandidates;
	GatherPerceptionTargetCandidates(CurrentlyPerceivedCandidates, KnownCandidates);
	return !CurrentlyPerceivedCandidates.IsEmpty();
}

AActor* AEnemyAIController::SelectBestTargetActorFromPerception() const
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(GetPawn()))
	{
		return nullptr;
	}

	TArray<AActor*> CurrentlyPerceivedCandidates;
	TArray<AActor*> KnownCandidates;
	GatherPerceptionTargetCandidates(CurrentlyPerceivedCandidates, KnownCandidates);

	if (KnownCandidates.IsEmpty())
	{
		// Perception events can be consumed while the leash is active; distance fallback prevents a dead idle after returning home.
		return SelectFallbackPlayerTargetByDistance();
	}

	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	const FName FocusTargetRule = IsValid(BlackboardComponent)
		? BlackboardComponent->GetValueAsName(EnemyBlackboardKeys::FocusTargetRule)
		: FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);

	// Fresh sight or sound should win over remembered stimuli, while the legacy editor setting remains compatible.
	const bool bUseCurrentlyPerceivedCandidatesFirst = bPreferCurrentlyVisibleTargets && !CurrentlyPerceivedCandidates.IsEmpty();
	const TArray<AActor*>& PreferredCandidates = bUseCurrentlyPerceivedCandidatesFirst ? CurrentlyPerceivedCandidates : KnownCandidates;

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat) && IsValid(BlackboardComponent))
	{
		AActor* CurrentTargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
		if (KnownCandidates.Contains(CurrentTargetActor))
		{
			return CurrentTargetActor;
		}
	}

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst))
	{
		if (AActor* PlayerFirstTarget = SelectPlayerFirstTargetCandidate(GetPawn(), PreferredCandidates))
		{
			return PlayerFirstTarget;
		}

		if (bUseCurrentlyPerceivedCandidatesFirst)
		{
			if (AActor* RememberedPlayerTarget = SelectPlayerFirstTargetCandidate(GetPawn(), KnownCandidates))
			{
				return RememberedPlayerTarget;
			}
		}

		return SelectNearestTargetCandidate(GetPawn(), PreferredCandidates);
	}

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest))
	{
		AActor* WeakestTargetActor = nullptr;
		float BestHealthRatio = TNumericLimits<float>::Max();
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : PreferredCandidates)
		{
			const float CandidateHealthRatio = GetCandidateHealthRatio(CandidateActor);
			const float DistanceSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), CandidateActor->GetActorLocation());
			if (CandidateHealthRatio < BestHealthRatio || (FMath::IsNearlyEqual(CandidateHealthRatio, BestHealthRatio) && DistanceSquared < BestDistanceSquared))
			{
				BestHealthRatio = CandidateHealthRatio;
				BestDistanceSquared = DistanceSquared;
				WeakestTargetActor = CandidateActor;
			}
		}

		return IsValid(WeakestTargetActor) ? WeakestTargetActor : SelectNearestTargetCandidate(GetPawn(), PreferredCandidates);
	}

	return SelectNearestTargetCandidate(GetPawn(), PreferredCandidates);
}

AActor* AEnemyAIController::SelectFallbackPlayerTargetByDistance() const
{
	const APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		return nullptr;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	if (!IsValid(PlayerPawn) || !IsValidPerceptionTarget(PlayerPawn))
	{
		return nullptr;
	}

	const float EffectiveSightRadius = IsValid(SightSenseConfig) ? SightSenseConfig->SightRadius : SightRadius;
	if (FVector::DistSquared2D(ControlledPawn->GetActorLocation(), PlayerPawn->GetActorLocation()) > FMath::Square(EffectiveSightRadius))
	{
		return nullptr;
	}

	// Keep AI Perception as the source of truth, but allow a fresh LOS check to recover after the leash suppresses perception callbacks.
	return LineOfSightTo(PlayerPawn) ? PlayerPawn : nullptr;
}

void AEnemyAIController::GatherPerceptionTargetCandidates(TArray<AActor*>& OutCurrentlyPerceivedCandidates, TArray<AActor*>& OutKnownCandidates) const
{
	OutCurrentlyPerceivedCandidates.Reset();
	OutKnownCandidates.Reset();

	if (!IsValid(EnemyPerceptionComponent))
	{
		return;
	}

	auto GatherCandidatesForSense = [this, &OutCurrentlyPerceivedCandidates, &OutKnownCandidates](TSubclassOf<UAISense> SenseClass)
	{
		TArray<AActor*> CurrentlyPerceivedActors;
		EnemyPerceptionComponent->GetCurrentlyPerceivedActors(SenseClass, CurrentlyPerceivedActors);
		for (AActor* CandidateActor : CurrentlyPerceivedActors)
		{
			if (IsValidPerceptionTarget(CandidateActor))
			{
				OutCurrentlyPerceivedCandidates.AddUnique(CandidateActor);
				OutKnownCandidates.AddUnique(CandidateActor);
			}
		}

		if (!bUseKnownSightTargetsForSelection)
		{
			return;
		}

		TArray<AActor*> KnownActors;
		EnemyPerceptionComponent->GetKnownPerceivedActors(SenseClass, KnownActors);
		for (AActor* CandidateActor : KnownActors)
		{
			if (IsValidPerceptionTarget(CandidateActor))
			{
				OutKnownCandidates.AddUnique(CandidateActor);
			}
		}
	};

	// Both senses intentionally feed one deduplicated candidate list for the existing focus-target rules.
	GatherCandidatesForSense(UAISense_Sight::StaticClass());
	GatherCandidatesForSense(UAISense_Hearing::StaticClass());
}

bool AEnemyAIController::IsValidPerceptionTarget(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor))
	{
		return false;
	}

	if (CandidateActor == GetPawn())
	{
		return false;
	}

	if (TargetActorClass && !CandidateActor->IsA(TargetActorClass))
	{
		return false;
	}

	if (bTargetOnlyPlayerControlledPawns)
	{
		const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
		if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
		{
			return false;
		}
	}

	return true;
}

float AEnemyAIController::GetCandidateHealthRatio(AActor* CandidateActor) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CandidateActor);
	if (!IsValid(ASC))
	{
		return 1.0f;
	}

	const UGP_AttributeSet* AttributeSet = ASC->GetSet<UGP_AttributeSet>();
	if (!IsValid(AttributeSet))
	{
		return 1.0f;
	}

	const float MaxHealth = AttributeSet->GetMaxHealth();
	if (MaxHealth <= KINDA_SMALL_NUMBER)
	{
		return 1.0f;
	}

	return FMath::Clamp(AttributeSet->GetHealth() / MaxHealth, 0.0f, 1.0f);
}

void AEnemyAIController::SetBlackboardTargetActor(AActor* NewTargetActor)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* CurrentTargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (CurrentTargetActor == NewTargetActor)
	{
		return;
	}

	if (!IsValid(CurrentTargetActor) && IsValid(NewTargetActor))
	{
		UE_LOG(LogEnemyAI, Log, TEXT("[Perception] Target acquired: %s"), *EnemyAIDebugUtils::DescribeActor(NewTargetActor));
	}
	else if (IsValid(CurrentTargetActor) && !IsValid(NewTargetActor))
	{
		UE_LOG(LogEnemyAI, Log, TEXT("[Perception] Target lost: %s"), *EnemyAIDebugUtils::DescribeActor(CurrentTargetActor));
	}
	else
	{
		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[Perception] Target changed: %s -> %s"),
			*EnemyAIDebugUtils::DescribeActor(CurrentTargetActor),
			*EnemyAIDebugUtils::DescribeActor(NewTargetActor));
	}

	if (IsValid(NewTargetActor))
	{
		BlackboardComponent->SetValueAsObject(EnemyBlackboardKeys::TargetActor, NewTargetActor);

		// Start the turn before this target change wakes the BT MoveTo branch. Otherwise
		// CharacterMovement can rotate the capsule before the turn montage gets a chance.
		if (AGP_EnemyCharacter* EnemyPawn = Cast<AGP_EnemyCharacter>(GetPawn()))
		{
			EnemyPawn->StartTurnInPlaceForTarget(NewTargetActor);
			EnemyPawn->NotifyBossTargetSelected(NewTargetActor);
		}
	}
	else
	{
		BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	}

	// Target changes should interrupt Patrol/Idle without asking BT services to restart themselves mid-tick.
	RequestBehaviorTreeRootReevaluation();
}

void AEnemyAIController::SetLeashReturnHomeActive(bool bActive)
{
	if (bLeashReturnHomeActive == bActive)
	{
		return;
	}

	bLeashReturnHomeActive = bActive;

	if (bLeashReturnHomeActive)
	{
		// Stop the current chase immediately; the BT return-home branch will own the next MoveTo.
		StopMovement();
		SetBlackboardTargetActor(nullptr);
		RequestBehaviorTreeRootReevaluation();
		UE_LOG(LogEnemyAI, Log, TEXT("[Leash] Return home started: %s"), *EnemyAIDebugUtils::DescribeActor(GetPawn()));
		return;
	}

	UE_LOG(LogEnemyAI, Log, TEXT("[Leash] Return home finished: %s"), *EnemyAIDebugUtils::DescribeActor(GetPawn()));
	if (IsValid(EnemyPerceptionComponent))
	{
		// A player may already be inside sight when the leash unlocks, so request a fresh stimuli pass before rescoring.
		EnemyPerceptionComponent->RequestStimuliListenerUpdate();
	}

	RequestTargetActorReevaluation();
	RequestBehaviorTreeRootReevaluation();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			// The next tick catches perception updates that arrive just after the leash flag is cleared.
			RequestTargetActorReevaluation();
			RequestBehaviorTreeRootReevaluation();
		}));
	}
}

void AEnemyAIController::RequestBehaviorTreeRootReevaluation()
{
	if (bHasPendingBehaviorTreeRootReevaluation)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		return;
	}

	bHasPendingBehaviorTreeRootReevaluation = true;
	World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		bHasPendingBehaviorTreeRootReevaluation = false;

		UBehaviorTreeComponent* BehaviorTreeComponent = Cast<UBehaviorTreeComponent>(GetBrainComponent());
		if (!IsValid(BehaviorTreeComponent) || !BehaviorTreeComponent->IsRunning())
		{
			PendingBehaviorTreeRootReevaluationRetryCount = 0;
			return;
		}

		if (BehaviorTreeComponent->IsAbortPending() || BehaviorTreeComponent->IsRestartPending())
		{
			// ReturnHome often flips state while BT is aborting MoveTo; retry instead of dropping the wake-up.
			if (++PendingBehaviorTreeRootReevaluationRetryCount <= 3)
			{
				RequestBehaviorTreeRootReevaluation();
			}
			else
			{
				PendingBehaviorTreeRootReevaluationRetryCount = 0;
			}
			return;
		}

		PendingBehaviorTreeRootReevaluationRetryCount = 0;

		// Restart on the next world tick so MoveTo/Wait can be replaced by the highest-priority valid branch.
		BehaviorTreeComponent->RestartTree(EBTRestartMode::ForceReevaluateRootNode);
	}));
}
