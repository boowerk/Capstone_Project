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

/** 媛꾨떒???뚮젅?댁뼱 ?됯? 吏??*/
USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FPlayerEvaluationSnapshot
{
    GENERATED_BODY()

    /** 0~1 ?ъ씠??怨듦꺽 ?깊뼢 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float AggressionScore = 0.5f;

    /** 0~1 ?ъ씠???먯깋/?대룞 ?깊뼢 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float ExplorationScore = 0.5f;

    /** 0~1 ?ъ씠???앹〈/諛⑹뼱 ?깊뼢 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SurvivalScore = 0.5f;

    /** 0~1 ?ъ씠??吏??援곗쨷?쒖뼱 ?좏샇??*/
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Evaluation")
    float SupportScore = 0.5f;

    /** JSON ?섏씠濡쒕뱶?먯꽌 ?먯닔瑜??쎌뼱? 援ъ“泥대? 梨꾩썎?덈떎. */
    static bool FromJson(const FString& JsonPayload, FPlayerEvaluationSnapshot& OutSnapshot);
};

/** ?⑥닚???됰룞 ?쇰꺼??濡쒓렇濡??④린???쒖뒪??*/
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

/** 媛??媛源뚯슫 ?뚮젅?댁뼱瑜??ν빐 ?대룞?섎뒗 ?쒖뒪??**/
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

/** 洹쇱젒 踰붿쐞?먯꽌 ?뚮젅?댁뼱?먭쾶 ?곕?吏瑜??곸슜?섎뒗 ?쒖뒪??**/
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
 * ?뚮젅?댁뼱 ?됯? JSON??湲곕컲?쇰줈 媛꾨떒???됰룞?몃━瑜??앹꽦?섎뒗 鍮뚮뜑
 */
UCLASS()
class PROJECT_EDEN_API UBTTask_DetectPlayer : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_DetectPlayer();

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float DetectionRange = 1000.0f;                    // ?뚮젅?댁뼱 媛먯? 諛섍꼍

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float MaxChaseDistance = 0.0f;                     // ???꾩튂 湲곗? 異붿쟻 ?좎? 諛섍꼍

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
    float WanderRadius = 700.0f;                         // 臾댁옉???대룞 諛섍꼍

    UPROPERTY(EditAnywhere, Category = "Behavior")
    float AcceptanceRadius = 100.0f;                    // ?대룞 紐⑺몴???꾨떖??寃껋쑝濡?媛꾩＜?섎뒗 諛섍꼍

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};


UCLASS(BlueprintType)
class PROJECT_EDEN_API UPlayerBehaviorTreeBuilder : public UObject
{
    GENERATED_BODY()

public:
    /** JSON 臾몄옄?댁쓣 諛쏆븘 ?고??꾩슜 ?됰룞?몃━ ?몄뒪?댁뒪瑜??앹꽦?⑸땲?? */
    UFUNCTION(BlueprintCallable, Category = "Behavior")
    UBehaviorTree* BuildBehaviorTreeFromJson(const FString& EvaluationJson);

    /** ?대? ?뚯떛???ㅻ깄?룹쓣 諛쏆븘 ?됰룞?몃━瑜?留뚮벊?덈떎. */
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