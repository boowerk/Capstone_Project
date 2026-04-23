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
	void ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const;
	bool ValidateSharedBlackboardSchema(UBlackboardComponent* BlackboardComponent);

	void ConfigureSightSense();
	void RequestTargetActorReevaluation();
	void RefreshTargetActorFromPerception();
	AActor* SelectBestTargetActorFromPerception() const;
	bool IsValidPerceptionTarget(AActor* CandidateActor) const;
	float GetCandidateHealthRatio(AActor* CandidateActor) const;
	void SetBlackboardTargetActor(AActor* NewTargetActor);

	FTimerHandle PendingEnemyEvaluationTimerHandle;
	FTimerHandle EvaluationRefreshTimerHandle;

	bool bHasPendingEnemyEvaluation = false;
	bool bHasLastAppliedEnemyEvaluation = false;
	bool bHasWarnedAboutMissingBlackboardKeys = false;
	float LastEnemyEvaluationApplyTime = -1000.0f;
	FEnemyLLMEvaluation PendingEnemyEvaluation;
	FEnemyLLMEvaluation LastAppliedEnemyEvaluation;
};
