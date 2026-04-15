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

	// 외부 시스템이 계산한 평가값을 안전하게 받아 Blackboard에 반영한다.
	bool SubmitEnemyEvaluation(const FEnemyLLMEvaluation& InEvaluation, bool bForceImmediate = false);

	// 이후 실제 LLM 응답이 JSON으로 들어오면 이 진입점을 통해 파싱 후 반영할 수 있다.
	bool SubmitEnemyEvaluationFromJson(const FString& JsonPayload, bool bForceImmediate = false);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 공용 Blackboard 에셋을 지정하면 같은 키 체계를 여러 적이 재사용할 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardData> DefaultBlackboardAsset;

	// 공용 Behavior Tree 에셋을 지정하면 같은 BT를 여러 적이 공유할 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTreeAsset;

	// 테스트 중에는 에디터 지정이 비어 있어도 프로젝트의 공용 BT/Blackboard를 자동으로 찾아 연결한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Testing")
	bool bUseSharedBehaviorAssetFallback = true;

	// AI Perception의 공용 진입점이다. 감지 결과를 모아 타겟 선정에 사용한다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> EnemyPerceptionComponent;

	// 현재 단계에서는 시야 감지만 사용하고, 나중에 청각 등 다른 센스를 추가할 수 있다.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightSenseConfig;

	// 적이 처음 대상을 감지하기 시작하는 기본 시야 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float SightRadius = 2000.0f;

	// 한 번 본 대상을 잃었다고 판정하기 전까지 유지하는 시야 반경이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float LoseSightRadius = 2400.0f;

	// 정면 기준 좌우 시야각이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float PeripheralVisionAngleDegrees = 70.0f;

	// 마지막으로 본 자극을 기억하는 시간이다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float SightMaxAge = 2.5f;

	// 마지막으로 본 위치 주변에서 자동 성공 판정을 유지할 거리다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0"))
	float AutoSuccessRangeFromLastSeenLocation = 600.0f;

	// 특별한 팀 시스템이 없더라도 기본적으로 플레이어 폰을 타겟으로 삼도록 한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bTargetOnlyPlayerControlledPawns = true;

	// 감지 가능한 타겟 클래스 범위를 제한하고 싶을 때 사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception")
	TSubclassOf<AActor> TargetActorClass;

	// 팀/태도 시스템을 붙일 때 에디터에서 감지 범위를 쉽게 바꿀 수 있도록 노출한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectEnemies = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectFriendlies = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Perception|Affiliation")
	bool bDetectNeutrals = true;

	// 평가값을 너무 자주 덮어쓰지 않도록 적용 최소 간격을 둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM", meta = (ClampMin = "0.1"))
	float MinEnemyEvaluationApplyInterval = 1.0f;

	// 실제 네트워크/LLM 요청을 붙일 때 사용할 권장 갱신 주기다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM", meta = (ClampMin = "0.1"))
	float EvaluationRefreshInterval = 2.0f;

	// 이후 단계에서 비동기 평가 요청 루프를 쉽게 켜고 끌 수 있게 남겨둔다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|LLM")
	bool bEnableEvaluationRefreshLoop = false;

	// 평가 함수가 없어도 테스트가 가능하도록 컨트롤러가 직접 전투 상태 키를 주기적으로 갱신한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Testing", meta = (ClampMin = "0.05"))
	float CombatStateUpdateInterval = 0.25f;

	// 테스트 중에는 BT 서비스가 비어 있어도 이 내장 갱신기로 분기 키를 유지한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI|Testing")
	bool bEnableBuiltInCombatStateUpdater = true;

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
	void HandleCombatStateUpdateTimerElapsed();
	void StartCombatStateUpdateLoop();
	void StopCombatStateUpdateLoop();
	void UpdateCombatStateBlackboard();
	float GetCurrentHealthRatio() const;
	void ResolveBehaviorAssets(APawn* InPawn, UBehaviorTree*& OutBehaviorTreeAsset, UBlackboardData*& OutBlackboardAsset) const;
	bool ValidateSharedBlackboardSchema(UBlackboardComponent* BlackboardComponent);

	void ConfigureSightSense();
	void RefreshTargetActorFromPerception();
	AActor* SelectBestTargetActorFromPerception() const;
	bool IsValidPerceptionTarget(AActor* CandidateActor) const;
	void SetBlackboardTargetActor(AActor* NewTargetActor);

	FTimerHandle PendingEnemyEvaluationTimerHandle;
	FTimerHandle EvaluationRefreshTimerHandle;
	FTimerHandle CombatStateUpdateTimerHandle;

	bool bHasPendingEnemyEvaluation = false;
	bool bHasLastAppliedEnemyEvaluation = false;
	bool bHasWarnedAboutMissingBlackboardKeys = false;
	float LastEnemyEvaluationApplyTime = -1000.0f;
	FEnemyLLMEvaluation PendingEnemyEvaluation;
	FEnemyLLMEvaluation LastAppliedEnemyEvaluation;
};
