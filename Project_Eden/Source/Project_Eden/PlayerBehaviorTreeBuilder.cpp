#include "PlayerBehaviorTreeBuilder.h"

#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Dom/JsonObject.h"
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
    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("ChaseTarget")));
    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("AttackTarget")));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.2f + (1.0f - Snapshot.AggressionScore) * 0.4f));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildExplorationBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("ScanPointsOfInterest")));
    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("Roam")));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.5f * (1.0f - Snapshot.ExplorationScore)));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildSurvivalBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("FindCover")));
    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("HealOrRecover")));
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