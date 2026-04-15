#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "EnemyAIController.generated.h"

class AActor;
class APawn;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UBehaviorTree;
class UBlackboardComponent;
class UBlackboardData;
struct FEnemyLLMEvaluation;

UCLASS()
class PROJECT_EDEN_API AEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	AEnemyAIController();

	bool ApplyEnemyEvaluationToBlackboard(const FEnemyLLMEvaluation& InEvaluation);

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;

	// 공용 Blackboard 에셋을 지정하면 같은 키 체계를 여러 적이 재사용할 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardData> DefaultBlackboardAsset;

	// 공용 Behavior Tree 에셋을 지정하면 같은 BT를 여러 적이 공유할 수 있다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTreeAsset;

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

	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	bool InitializeBehaviorTree(APawn* InPawn);
	virtual void InitializeBlackboardValues(APawn* InPawn);
	UBlackboardComponent* GetEnemyBlackboardComponent();

	void ConfigureSightSense();
	void RefreshTargetActorFromPerception();
	AActor* SelectBestTargetActorFromPerception() const;
	bool IsValidPerceptionTarget(AActor* CandidateActor) const;
	void SetBlackboardTargetActor(AActor* NewTargetActor);
};
