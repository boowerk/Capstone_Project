#include "AI/PlayerBehaviorTreeBuilder.h"

#include "AIController.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Dom/JsonObject.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"

namespace
{
    float ClampScore(const TSharedPtr<FJsonObject>& Json, const FString& FieldName)
    {
        double Value = 0.5;
        if (Json->TryGetNumberField(FieldName, Value))
        {
            return FMath::Clamp(static_cast<float>(Value), 0.0f, 1.0f);
        }

        return 0.5f;
    }

    ACharacter* GetPlayerCharacter(UBehaviorTreeComponent& OwnerComp)
    {
        UWorld* World = OwnerComp.GetWorld();
		return IsValid(World) ? UGameplayStatics::GetPlayerCharacter(World, 0) : nullptr;
    }
}

bool FPlayerEvaluationSnapshot::FromJson(const FString& JsonPayload, FPlayerEvaluationSnapshot& OutSnapshot)
{
    TSharedPtr<FJsonObject> Root;
    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonPayload);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Warning, TEXT("[BehaviorBuilder] Invalid evaluation JSON: %s"), *JsonPayload);
        return false;
    }

    OutSnapshot.AggressionScore = ClampScore(Root, TEXT("aggression"));
    OutSnapshot.ExplorationScore = ClampScore(Root, TEXT("exploration"));
    OutSnapshot.SurvivalScore = ClampScore(Root, TEXT("survival"));
    OutSnapshot.SupportScore = ClampScore(Root, TEXT("support"));

    return true;
}

UBTTask_LogAction::UBTTask_LogAction()
{
    NodeName = TEXT("LogAction");
}

EBTNodeResult::Type UBTTask_LogAction::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    const FString ResolvedLabel = ActionLabel.IsEmpty() ? TEXT("BehaviorStep") : ActionLabel;
    UE_LOG(LogTemp, Log, TEXT("[BehaviorTree] %s executed"), *ResolvedLabel);
    return EBTNodeResult::Succeeded;
}

FString UBTTask_LogAction::GetStaticDescription() const
{
    return FString::Printf(TEXT("Log: %s"), *ActionLabel);
}

UBTTask_MoveToPlayer::UBTTask_MoveToPlayer()
{
    NodeName = TEXT("MoveToPlayer");
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_MoveToPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    ACharacter* TargetCharacter = GetPlayerCharacter(OwnerComp);
    if (!IsValid(AIController) || !IsValid(OwnerPawn) || !IsValid(TargetCharacter))
    {
        return EBTNodeResult::Failed;
    }

    const float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), TargetCharacter->GetActorLocation());
    if (Distance <= AcceptanceRadius)
    {
        return EBTNodeResult::Succeeded;
    }

    AIController->MoveToActor(TargetCharacter, AcceptanceRadius, true, true, true, 0, true);
    return EBTNodeResult::InProgress;
}

void UBTTask_MoveToPlayer::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    ACharacter* TargetCharacter = GetPlayerCharacter(OwnerComp);
    if (!IsValid(AIController) || !IsValid(OwnerPawn) || !IsValid(TargetCharacter))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), TargetCharacter->GetActorLocation());
    if (Distance <= AcceptanceRadius)
    {
        AIController->StopMovement();
        UE_LOG(LogTemp, Log, TEXT("[BehaviorTree] ChaseTarget reached player"));
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}

UBTTask_AttackPlayer::UBTTask_AttackPlayer()
{
    NodeName = TEXT("AttackPlayer");
}

EBTNodeResult::Type UBTTask_AttackPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    ACharacter* TargetCharacter = GetPlayerCharacter(OwnerComp);
    if (!IsValid(AIController) || !IsValid(OwnerPawn) || !IsValid(TargetCharacter))
    {
        return EBTNodeResult::Failed;
    }

    const float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), TargetCharacter->GetActorLocation());
    if (Distance > AttackRange)
    {
        return EBTNodeResult::Failed;
    }

    UGameplayStatics::ApplyDamage(TargetCharacter, DamageAmount, AIController, OwnerPawn, nullptr);
    UE_LOG(LogTemp, Log, TEXT("[BehaviorTree] AttackTarget executed. Damage=%.1f"), DamageAmount);
    return EBTNodeResult::Succeeded;
}

UBehaviorTree* UPlayerBehaviorTreeBuilder::BuildBehaviorTreeFromJson(const FString& EvaluationJson)
{
    FPlayerEvaluationSnapshot Snapshot;
    if (!FPlayerEvaluationSnapshot::FromJson(EvaluationJson, Snapshot))
    {
        return nullptr;
    }

    return BuildBehaviorTreeFromSnapshot(Snapshot);
}

UBehaviorTree* UPlayerBehaviorTreeBuilder::BuildBehaviorTreeFromSnapshot(const FPlayerEvaluationSnapshot& Snapshot)
{
    UBehaviorTree* BehaviorTree = NewObject<UBehaviorTree>(this, FName(TEXT("DynamicBehaviorTree")));
    UBTComposite_Selector* RootSelector = NewObject<UBTComposite_Selector>(BehaviorTree);
    BehaviorTree->RootNode = RootSelector;

    // 우선순위를 점수에 따라 정렬한 후 셀렉터에 추가합니다.
    TArray<TPair<float, UBTComposite_Sequence*>> OrderedBranches;
    OrderedBranches.Emplace(Snapshot.AggressionScore, BuildAggressiveBranch(BehaviorTree, Snapshot));
    OrderedBranches.Emplace(Snapshot.ExplorationScore, BuildExplorationBranch(BehaviorTree, Snapshot));
    OrderedBranches.Emplace(Snapshot.SurvivalScore, BuildSurvivalBranch(BehaviorTree, Snapshot));
    OrderedBranches.Emplace(Snapshot.SupportScore, BuildSupportBranch(BehaviorTree, Snapshot));
    OrderedBranches.Sort([](const TPair<float, UBTComposite_Sequence*>& A, const TPair<float, UBTComposite_Sequence*>& B)
        {
            return A.Key > B.Key;
        });

    for (const TPair<float, UBTComposite_Sequence*>& BranchPair : OrderedBranches)
    {
        AddChildToComposite(RootSelector, BranchPair.Value);
    }

    return BehaviorTree;
}

void UPlayerBehaviorTreeBuilder::AddChildToComposite(UBTCompositeNode* Parent, UBTNode* ChildNode) const
{
    if (!Parent || !ChildNode)
    {
        return;
    }

    FBTCompositeChild& Child = Parent->Children.AddDefaulted_GetRef();
    Child.ChildComposite = Cast<UBTCompositeNode>(ChildNode);
    Child.ChildTask = Cast<UBTTaskNode>(ChildNode);
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildAggressiveBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("AcquireTarget")));
    AddChildToComposite(Sequence, CreateMoveToPlayerTask(TreeOuter, 100.0f + (1.0f - Snapshot.AggressionScore) * 140.0f));
    AddChildToComposite(Sequence, CreateAttackPlayerTask(TreeOuter, 220.0f, 8.0f + Snapshot.AggressionScore * 12.0f));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.2f + (1.0f - Snapshot.AggressionScore) * 0.4f));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildExplorationBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("ScanPointsOfInterest")));
    AddChildToComposite(Sequence, CreateMoveToPlayerTask(TreeOuter, 550.0f + (1.0f - Snapshot.ExplorationScore) * 350.0f));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.5f * (1.0f - Snapshot.ExplorationScore)));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildSurvivalBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("FindCover")));
    AddChildToComposite(Sequence, CreateAttackPlayerTask(TreeOuter, 300.0f, 4.0f + Snapshot.SupportScore * 6.0f));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.6f + Snapshot.SurvivalScore * 0.6f));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildSupportBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("BuffAllies")));
    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("DebuffEnemies")));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.3f + (1.0f - Snapshot.SupportScore) * 0.5f));

    return Sequence;
}

UBTTask_Wait* UPlayerBehaviorTreeBuilder::CreateWaitTask(UBehaviorTree* TreeOuter, float WaitTimeSeconds) const
{
    UBTTask_Wait* WaitTask = NewObject<UBTTask_Wait>(TreeOuter);
    WaitTask->WaitTime = WaitTimeSeconds;
    WaitTask->RandomDeviation = 0.0f;
    return WaitTask;
}

UBTTask_LogAction* UPlayerBehaviorTreeBuilder::CreateLogTask(UBehaviorTree* TreeOuter, const FString& Label) const
{
    UBTTask_LogAction* Task = NewObject<UBTTask_LogAction>(TreeOuter);
    Task->ActionLabel = Label;
    return Task;
}

UBTTask_MoveToPlayer* UPlayerBehaviorTreeBuilder::CreateMoveToPlayerTask(UBehaviorTree* TreeOuter, float AcceptanceRadius) const
{
    UBTTask_MoveToPlayer* Task = NewObject<UBTTask_MoveToPlayer>(TreeOuter);
    Task->AcceptanceRadius = AcceptanceRadius;
    return Task;
}

UBTTask_AttackPlayer* UPlayerBehaviorTreeBuilder::CreateAttackPlayerTask(UBehaviorTree* TreeOuter, float AttackRange, float DamageAmount) const
{
    UBTTask_AttackPlayer* Task = NewObject<UBTTask_AttackPlayer>(TreeOuter);
    Task->AttackRange = AttackRange;
    Task->DamageAmount = DamageAmount;
    return Task;
}