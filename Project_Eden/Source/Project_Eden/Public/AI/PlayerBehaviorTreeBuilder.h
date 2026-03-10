#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTTaskNode.h"
#include "PlayerBehaviorTreeBuilder.generated.h"

class UBehaviorTree;
class UBTNode;
class UBTCompositeNode;
class UBTComposite_Sequence;
class UBTTask_Wait;

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FPlayerEvaluationSnapshot
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float AggressionScore = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float ExplorationScore = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SurvivalScore = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SupportScore = 0.5f;

    static bool FromJson(const FString& JsonPayload, FPlayerEvaluationSnapshot& OutSnapshot);
};

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

UCLASS()
class PROJECT_EDEN_API UBTTask_MoveToPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_MoveToPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 140.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

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

UCLASS()
class PROJECT_EDEN_API UBTTask_DetectPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_DetectPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float DetectionRange = 1200.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};

UCLASS()
class PROJECT_EDEN_API UBTTask_Wander : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_Wander();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float WanderRadius = 700.0f;

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 90.0f;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UPlayerBehaviorTreeBuilder : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Behavior")
    UBehaviorTree* BuildBehaviorTreeFromJson(const FString& EvaluationJson);

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
    UBTTask_MoveToPlayer* CreateMoveToPlayerTask(UBehaviorTree* TreeOuter, float AcceptanceRadius) const;
    UBTTask_AttackPlayer* CreateAttackPlayerTask(UBehaviorTree* TreeOuter, float AttackRange, float DamageAmount) const;
    UBTTask_DetectPlayer* CreateDetectPlayerTask(UBehaviorTree* TreeOuter, float DetectionRange) const;
    UBTTask_Wander* CreateWanderTask(UBehaviorTree* TreeOuter, float WanderRadius, float AcceptanceRadius = 90.0f) const;
};
