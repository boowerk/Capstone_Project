#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "EnemyAIController.generated.h"

class APawn;
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

	// Step 2부터 공용 Blackboard 에셋을 지정하면 여기서 같은 키 체계를 재사용한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBlackboardData> DefaultBlackboardAsset;

	// Step 2부터 공용 Behavior Tree 에셋을 지정하면 여기서 적 공통 트리를 실행한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> DefaultBehaviorTreeAsset;

	bool InitializeBehaviorTree(APawn* InPawn);
	virtual void InitializeBlackboardValues(APawn* InPawn);

	UBlackboardComponent* GetEnemyBlackboardComponent();
};
