#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PlayerBehaviorTreeBuilder.generated.h"

class UBehaviorTree;
class UBTNode;
class UBTCompositeNode;
class UBTComposite_Selector;
class UBTComposite_Sequence;
class UBTTask_Wait;

/** 간단한 플레이어 평가 지표 */
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FPlayerEvaluationSnapshot
{
    GENERATED_BODY()

    /** 0~1 사이의 공격 성향 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float AggressionScore = 0.5f;

    /** 0~1 사이의 탐색/이동 성향 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float ExplorationScore = 0.5f;

    /** 0~1 사이의 생존/방어 성향 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SurvivalScore = 0.5f;

    /** 0~1 사이의 지원/군중제어 선호도 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SupportScore = 0.5f;

    /** JSON 페이로드에서 점수를 읽어와 구조체를 채웁니다. */
    static bool FromJson(const FString& JsonPayload, FPlayerEvaluationSnapshot& OutSnapshot);
};

/** 단순히 행동 라벨을 로그로 남기는 태스크 */
UCLASS()
class PROJECT_EDEN_API UBTTask_LogAction : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_LogAction();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    FString ActionLabel;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual FString GetStaticDescription() const override;
};

/**
 * 플레이어 평가 JSON을 기반으로 간단한 행동트리를 생성하는 빌더
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UPlayerBehaviorTreeBuilder : public UObject
{
    GENERATED_BODY()

public:
    /** JSON 문자열을 받아 런타임용 행동트리 인스턴스를 생성합니다. */
    UFUNCTION(BlueprintCallable, Category = "Behavior")
    UBehaviorTree* BuildBehaviorTreeFromJson(const FString& EvaluationJson);

    /** 이미 파싱된 스냅샷을 받아 행동트리를 만듭니다. */
    UFUNCTION(BlueprintCallable, Category = "Behavior")
    UBehaviorTree* BuildBehaviorTreeFromSnapshot(const FPlayerEvaluationSnapshot& Snapshot);

private:
    void AddChildToComposite(UBTCompositeNode* Parent, UBTNode* ChildNode) const;

    UBTComposite_Sequence* BuildAggressiveBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const;
    UBTComposite_Sequence* BuildExplorationBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const;
    UBTComposite_Sequence* BuildSurvivalBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const;
    UBTComposite_Sequence* BuildSupportBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const;

    UBTTask_Wait* CreateWaitTask(UBehaviorTree* TreeOuter, float WaitTimeSeconds) const;
    UBTTask_LogAction* CreateLogTask(UBehaviorTree* TreeOuter, const FString& Label) const;
};