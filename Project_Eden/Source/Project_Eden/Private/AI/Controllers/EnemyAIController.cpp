#include "AI/Controllers/EnemyAIController.h"

#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
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
		return TEXT("/Game/Characters/EnemyCharacter/BT/BT_EnemyCommon.BT_EnemyCommon");
	}

	const TCHAR* GetSharedBlackboardFallbackPath()
	{
		return TEXT("/Game/Characters/EnemyCharacter/BT/BB_EnemyCommon.BB_EnemyCommon");
	}
}

AEnemyAIController::AEnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	SightSenseConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightSenseConfig"));

	// Keep sensing on the engine perception component and keep this controller as the target selector.
	SetPerceptionComponent(*EnemyPerceptionComponent);

	TargetActorClass = APawn::StaticClass();

	if (IsValid(EnemyPerceptionComponent))
	{
		EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	ConfigureSightSense();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	UE_LOG(LogEnemyAI, Log, TEXT("[AI] Possess: Controller=%s Pawn=%s"), *GetName(), *EnemyAIDebugUtils::DescribeActor(InPawn));

	ConfigureSightSense();
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
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, InPawn->GetActorLocation());

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
	// Future hook: request an async LLM update, then call SubmitEnemyEvaluation with the response.
}

void AEnemyAIController::StartEvaluationRefreshLoop()
{
	if (!bEnableEvaluationRefreshLoop || EvaluationRefreshInterval <= 0.0f)
	{
		return;
	}

	// Keep refreshes coarse-grained; Behavior Tree services handle moment-to-moment tactics.
	GetWorldTimerManager().SetTimer(
		EvaluationRefreshTimerHandle,
		this,
		&ThisClass::HandleEvaluationRefreshTimerElapsed,
		EvaluationRefreshInterval,
		true);
}

void AEnemyAIController::StopEvaluationRefreshLoop()
{
	if (GetWorld() != nullptr)
	{
		GetWorldTimerManager().ClearTimer(EvaluationRefreshTimerHandle);
	}
}

void AEnemyAIController::ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const
{
	OutBehaviorTreeAsset = DefaultBehaviorTreeAsset;
	OutBlackboardAsset = DefaultBlackboardAsset;

	if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(InPawn))
	{
		if (!IsValid(OutBehaviorTreeAsset))
		{
			OutBehaviorTreeAsset = EnemyCharacter->GetBehaviorTreeAssetOverride();
		}

		if (!IsValid(OutBlackboardAsset))
		{
			OutBlackboardAsset = EnemyCharacter->GetBlackboardAssetOverride();
		}
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
		EnemyBlackboardKeys::MoveToLocation,
		EnemyBlackboardKeys::FocusTargetRule,
		EnemyBlackboardKeys::bShouldRetreat,
		EnemyBlackboardKeys::bCanAttack,
		EnemyBlackboardKeys::bShouldReposition,
		EnemyBlackboardKeys::bShouldChase,
		EnemyBlackboardKeys::bHasLineOfSight,
		EnemyBlackboardKeys::DistanceToTarget,
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

	SightSenseConfig->SightRadius = SightRadius;
	SightSenseConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightSenseConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
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
	// Both perception events and tuning changes use this path to re-score currently perceived candidates.
	RefreshTargetActorFromPerception();
}

void AEnemyAIController::RefreshTargetActorFromPerception()
{
	SetBlackboardTargetActor(SelectBestTargetActorFromPerception());
}

AActor* AEnemyAIController::SelectBestTargetActorFromPerception() const
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(GetPawn()))
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	TArray<AActor*> Candidates;
	for (AActor* CandidateActor : PerceivedActors)
	{
		if (IsValidPerceptionTarget(CandidateActor))
		{
			Candidates.Add(CandidateActor);
		}
	}

	if (Candidates.IsEmpty())
	{
		return nullptr;
	}

	const UBlackboardComponent* BlackboardComponent = GetBlackboardComponent();
	const FName FocusTargetRule = IsValid(BlackboardComponent)
		? BlackboardComponent->GetValueAsName(EnemyBlackboardKeys::FocusTargetRule)
		: FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat);

	const auto SelectNearestCandidate = [this, &Candidates]() -> AActor*
	{
		AActor* BestTargetActor = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : Candidates)
		{
			const float DistanceSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), CandidateActor->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestTargetActor = CandidateActor;
			}
		}

		return BestTargetActor;
	};

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::CurrentThreat) && IsValid(BlackboardComponent))
	{
		AActor* CurrentTargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
		if (Candidates.Contains(CurrentTargetActor))
		{
			return CurrentTargetActor;
		}
	}

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::PlayerFirst))
	{
		AActor* BestPlayerActor = nullptr;
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : Candidates)
		{
			const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
			if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
			{
				continue;
			}

			const float DistanceSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), CandidateActor->GetActorLocation());
			if (DistanceSquared < BestDistanceSquared)
			{
				BestDistanceSquared = DistanceSquared;
				BestPlayerActor = CandidateActor;
			}
		}

		return IsValid(BestPlayerActor) ? BestPlayerActor : SelectNearestCandidate();
	}

	if (FocusTargetRule == FEnemyLLMEvaluationParser::ToBlackboardName(EEnemyFocusTargetRule::Weakest))
	{
		AActor* WeakestTargetActor = nullptr;
		float BestHealthRatio = TNumericLimits<float>::Max();
		float BestDistanceSquared = TNumericLimits<float>::Max();

		for (AActor* CandidateActor : Candidates)
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

		return IsValid(WeakestTargetActor) ? WeakestTargetActor : SelectNearestCandidate();
	}

	return SelectNearestCandidate();
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
	}
	else
	{
		BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	}
}
