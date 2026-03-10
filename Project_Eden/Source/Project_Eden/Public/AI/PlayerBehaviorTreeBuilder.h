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

/** 가장 가까운 플레이어를 향해 이동하는 태스크 **/
UCLASS()
class PROJECT_EDEN_API UBTTask_MoveToPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_MoveToPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 140.0f;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float MaxChaseDistance = 0.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

/** 근접 범위에서 플레이어에게 데미지를 적용하는 태스크 **/
UCLASS()
class PROJECT_EDEN_API UBTTask_AttackPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_AttackPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AttackRange = 220.0f;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float DamageAmount = 10.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

/**
 * 플레이어 평가 JSON을 기반으로 간단한 행동트리를 생성하는 빌더
 */
UCLASS()
class PROJECT_EDEN_API UBTTask_DetectPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_DetectPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float DetectionRange = 1000.0f;                    // 플레이어 감지 반경

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float MaxChaseDistance = 0.0f;                     // 홈 위치 기준 추적 유지 반경

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class PROJECT_EDEN_API UBTTask_ReturnToAnchor : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ReturnToAnchor();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 120.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS()
class PROJECT_EDEN_API UBTTask_Wander : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Wander();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float WanderRadius = 700.0f;                         // 무작위 이동 반경

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 100.0f;                    // 이동 목표에 도달한 것으로 간주하는 반경

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};


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

    UBTTask_MoveToPlayer* CreateMoveToPlayerTask(UBehaviorTree* TreeOuter, float AcceptanceRadius, float MaxChaseDistance = 0.0f) const;
    UBTTask_AttackPlayer* CreateAttackPlayerTask(UBehaviorTree* TreeOuter, float AttackRange, float DamageAmount) const;
    UBTTask_DetectPlayer* CreateDetectPlayerTask(UBehaviorTree* TreeOuter, float DetectionRange, float MaxChaseDistance = 0.0f) const;
    UBTTask_ReturnToAnchor* CreateReturnToAnchorTask(UBehaviorTree* TreeOuter, float AcceptanceRadius = 120.0f) const;
    UBTTask_Wander* CreateWanderTask(UBehaviorTree* TreeOuter, float WanderRadius, float AcceptanceRadius = 90.0f) const;
};