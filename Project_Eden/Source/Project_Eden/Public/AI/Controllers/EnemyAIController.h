#pragma once

#include "AI/Data/EnemyLLMEvaluation.h"
#include "AIController.h"
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class AActor;
class APawn;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardComponent;
class UBlackboardData;

UCLASS()
class PROJECT_EDEN_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	// Applies high-level LLM/archetype tuning values to Blackboard; tactical bools are owned by BT services.
	bool SubmitEnemyEvaluation(const FEnemyLLMEvaluation& InEvaluation, bool bForceImmediate = false);

	// Keeps the JSON entry point narrow: JSON may change tuning values, not direct branch decisions.
	bool SubmitEnemyEvaluationFromJson(const FString& JsonPayload, bool bForceImmediate = false);

	// BT services use this leash state to pause target reacquisition until the pawn has returned home.
	void SetLeashReturnHomeActive(bool bActive);
	bool IsLeashReturnHomeActive() const { return bLeashReturnHomeActive; }

	// BT services can ask perception to rescore targets after a leash state or tactical state change.
	void RequestTargetActorReevaluation();
	bool HasCurrentlyPerceivedTargetCandidate() const;

	// Boss tactics services use this to open deterministic pattern windows while validating runtime evaluation changes.
	bool IsBossRuntimeEvaluationTestCycleActive() const;

	// Schedules a safe root re-evaluation outside BehaviorTreeComponent tick.
	void RequestBehaviorTreeRootReevaluation();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// Prefer explicit editor assignment; the fallback path is kept only for local testing.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardData> DefaultBlackboardAsset;

	// Designers should assign the shared or archetype-specific BT asset in the editor.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTreeAsset;

	// Development-only escape hatch for test maps that have not wired controller assets yet.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Testing")
	bool bUseSharedBehaviorAssetFallback = false;

	// AI Perception owns sensing; this controller only converts perceived candidates into TargetActor.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	// Sight is the first supported sense; add other sense configs here only when gameplay needs them.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightSenseConfig;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 2400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float SightMaxAge = 2.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float AutoSuccessRangeFromLastSeenLocation = 600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bTargetOnlyPlayerControlledPawns = true;

	// Prefer remembered sight targets so BT does not thrash when sight flickers for a frame.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bUseKnownSightTargetsForSelection = true;

	// When both visible and remembered candidates exist, visible ones should win unless designers disable it.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bPreferCurrentlyVisibleTargets = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	TSubclassOf<AActor> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectFriendlies = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectNeutrals = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM", meta = (ClampMin = "0.1"))
	float MinEnemyEvaluationApplyInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM", meta = (ClampMin = "0.1"))
	float EvaluationRefreshInterval = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM")
	bool bEnableEvaluationRefreshLoop = false;

	// Local boss test mode cycles evaluation values so each boss attack pattern can be observed in play.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM|Testing")
	bool bEnableBossRuntimeEvaluationTestCycle = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM|Testing", meta = (ClampMin = "0.25"))
	float BossRuntimeEvaluationTestInterval = 2.5f;

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool InitializeBehaviorTree(APawn* InPawn);
	virtual void InitializeBlackboardValues(APawn* InPawn);
	UBlackboardComponent* GetEnemyBlackboardComponent();
	bool ApplyEnemyEvaluationToBlackboard(const FEnemyLLMEvaluation& InEvaluation);
	bool ApplyEnemyEvaluationInternal(const FEnemyLLMEvaluation& InEvaluation);
	void HandlePendingEnemyEvaluationTimerElapsed();
	void HandleEvaluationRefreshTimerElapsed();
	void StartEvaluationRefreshLoop();
	void StopEvaluationRefreshLoop();
	bool ShouldUseBossRuntimeEvaluationTestCycle() const;
	void ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const;
	bool ValidateSharedBlackboardSchema(UBlackboardComponent* BlackboardComponent);

	void ConfigureSightSense();
	void RefreshTargetActorFromPerception();
	void GatherPerceptionTargetCandidates(TArray<AActor*>& OutVisibleCandidates, TArray<AActor*>& OutKnownCandidates) const;
	AActor* SelectBestTargetActorFromPerception() const;
	AActor* SelectFallbackPlayerTargetByDistance() const;
	bool IsValidPerceptionTarget(AActor* CandidateActor) const;
	float GetCandidateHealthRatio(AActor* CandidateActor) const;
	void SetBlackboardTargetActor(AActor* NewTargetActor);

	FTimerHandle PendingEnemyEvaluationTimerHandle;
	FTimerHandle EvaluationRefreshTimerHandle;

	bool bHasPendingEnemyEvaluation = false;
	bool bHasLastAppliedEnemyEvaluation = false;
	bool bHasWarnedAboutMissingBlackboardKeys = false;
	bool bLeashReturnHomeActive = false;
	bool bHasPendingBehaviorTreeRootReevaluation = false;
	int32 PendingBehaviorTreeRootReevaluationRetryCount = 0;
	int32 BossRuntimeEvaluationTestIndex = 0;
	float LastEnemyEvaluationApplyTime = -1000.0f;
	FEnemyLLMEvaluation PendingEnemyEvaluation;
	FEnemyLLMEvaluation LastAppliedEnemyEvaluation;
};
