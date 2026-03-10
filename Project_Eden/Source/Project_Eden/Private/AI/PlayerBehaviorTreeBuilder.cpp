#include "AI/PlayerBehaviorTreeBuilder.h"

#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "Dom/JsonObject.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

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
        UE_LOG(LogTemp, Verbose, TEXT("[BehaviorTree] AttackPlayer failed (out of range)"));
        return EBTNodeResult::Failed;
    }

    UGameplayStatics::ApplyDamage(TargetCharacter, DamageAmount, AIController, OwnerPawn, nullptr);
    UE_LOG(LogTemp, Log, TEXT("[BehaviorTree] AttackTarget executed. Damage=%.1f"), DamageAmount);
    return EBTNodeResult::Succeeded;
}

UBTTask_DetectPlayer::UBTTask_DetectPlayer()
{
    NodeName = TEXT("DetectPlayer");
}

EBTNodeResult::Type UBTTask_DetectPlayer::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    ACharacter* TargetCharacter = GetPlayerCharacter(OwnerComp);
    if (!IsValid(AIController) || !IsValid(OwnerPawn) || !IsValid(TargetCharacter))
    {
        return EBTNodeResult::Failed;
    }

    const float Distance = FVector::Dist(OwnerPawn->GetActorLocation(), TargetCharacter->GetActorLocation());
    const bool bInRange = Distance <= DetectionRange;
    const bool bHasLineOfSight = AIController->LineOfSightTo(TargetCharacter);
    const bool bDetected = bInRange && bHasLineOfSight;

    UE_LOG(LogTemp, Verbose, TEXT("[BehaviorTree] DetectPlayer range=%.1f inRange=%d LOS=%d"), Distance, bInRange ? 1 : 0, bHasLineOfSight ? 1 : 0);
    return bDetected ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

UBTTask_Wander::UBTTask_Wander()
{
    NodeName = TEXT("Wander");
    bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_Wander::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    UWorld* World = OwnerComp.GetWorld();
    if (!IsValid(AIController) || !IsValid(OwnerPawn) || !IsValid(World))
    {
        return EBTNodeResult::Failed;
    }

    UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!IsValid(NavSystem))
    {
        return EBTNodeResult::Failed;
    }

    FNavLocation RandomLocation;
    const bool bFoundPoint = NavSystem->GetRandomReachablePointInRadius(OwnerPawn->GetActorLocation(), WanderRadius, RandomLocation);
    if (!bFoundPoint)
    {
        return EBTNodeResult::Failed;
    }

    const EPathFollowingRequestResult::Type MoveResult = AIController->MoveToLocation(RandomLocation.Location, AcceptanceRadius, true, true, true, true, nullptr, true);
    if (MoveResult == EPathFollowingRequestResult::Failed)
    {
        return EBTNodeResult::Failed;
    }

    UE_LOG(LogTemp, Verbose, TEXT("[BehaviorTree] Wander started to %s"), *RandomLocation.Location.ToString());
    return EBTNodeResult::InProgress;
}

void UBTTask_Wander::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    APawn* OwnerPawn = AIController ? AIController->GetPawn() : nullptr;
    if (!IsValid(AIController) || !IsValid(OwnerPawn))
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    const EPathFollowingStatus::Type MoveStatus = AIController->GetMoveStatus();
    if (MoveStatus == EPathFollowingStatus::Idle)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
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
    UBTComposite_Sequence* Branch = NewObject<UBTComposite_Sequence>(TreeOuter);

    const float DetectionRange = 1000.0f + Snapshot.AggressionScore * 1200.0f;
    const float WanderRadius = 450.0f + Snapshot.ExplorationScore * 800.0f;

    AddChildToComposite(Branch, CreateLogTask(TreeOuter, TEXT("AggressiveMode")));

    UBTComposite_Selector* EngageOrWander = NewObject<UBTComposite_Selector>(TreeOuter);
    AddChildToComposite(Branch, EngageOrWander);

    UBTComposite_Sequence* EngageSequence = NewObject<UBTComposite_Sequence>(TreeOuter);
    AddChildToComposite(EngageOrWander, EngageSequence);
    AddChildToComposite(EngageSequence, CreateDetectPlayerTask(TreeOuter, DetectionRange));
    AddChildToComposite(EngageSequence, CreateMoveToPlayerTask(TreeOuter, 100.0f + (1.0f - Snapshot.AggressionScore) * 140.0f));

    UBTComposite_Selector* AttackOrWander = NewObject<UBTComposite_Selector>(TreeOuter);
    AddChildToComposite(EngageSequence, AttackOrWander);
    AddChildToComposite(AttackOrWander, CreateAttackPlayerTask(TreeOuter, 220.0f, 8.0f + Snapshot.AggressionScore * 12.0f));
    AddChildToComposite(AttackOrWander, CreateWanderTask(TreeOuter, WanderRadius));

    AddChildToComposite(EngageOrWander, CreateWanderTask(TreeOuter, WanderRadius));
    AddChildToComposite(Branch, CreateWaitTask(TreeOuter, 0.2f + (1.0f - Snapshot.AggressionScore) * 0.4f));

    return Branch;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildExplorationBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    const float DetectionRange = 700.0f + Snapshot.ExplorationScore * 1500.0f;
    const float WanderRadius = 800.0f + Snapshot.ExplorationScore * 1200.0f;

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("ExploreAndHunt")));

    UBTComposite_Selector* SearchOrEngage = NewObject<UBTComposite_Selector>(TreeOuter);
    AddChildToComposite(Sequence, SearchOrEngage);

    UBTComposite_Sequence* EngageWhenDetected = NewObject<UBTComposite_Sequence>(TreeOuter);
    AddChildToComposite(SearchOrEngage, EngageWhenDetected);
    AddChildToComposite(EngageWhenDetected, CreateDetectPlayerTask(TreeOuter, DetectionRange));
    AddChildToComposite(EngageWhenDetected, CreateMoveToPlayerTask(TreeOuter, 220.0f));
    AddChildToComposite(EngageWhenDetected, CreateAttackPlayerTask(TreeOuter, 260.0f, 5.0f + Snapshot.AggressionScore * 7.0f));

    AddChildToComposite(SearchOrEngage, CreateWanderTask(TreeOuter, WanderRadius));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.2f + 0.5f * (1.0f - Snapshot.ExplorationScore)));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildSurvivalBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("SurviveThenProbe")));
    AddChildToComposite(Sequence, CreateWanderTask(TreeOuter, 350.0f + Snapshot.SurvivalScore * 450.0f));
    AddChildToComposite(Sequence, CreateDetectPlayerTask(TreeOuter, 500.0f + Snapshot.SurvivalScore * 700.0f));
    AddChildToComposite(Sequence, CreateAttackPlayerTask(TreeOuter, 300.0f, 4.0f + Snapshot.SupportScore * 6.0f));
    AddChildToComposite(Sequence, CreateWaitTask(TreeOuter, 0.6f + Snapshot.SurvivalScore * 0.6f));

    return Sequence;
}

UBTComposite_Sequence* UPlayerBehaviorTreeBuilder::BuildSupportBranch(UBehaviorTree* TreeOuter, const FPlayerEvaluationSnapshot& Snapshot) const
{
    UBTComposite_Sequence* Sequence = NewObject<UBTComposite_Sequence>(TreeOuter);

    AddChildToComposite(Sequence, CreateLogTask(TreeOuter, TEXT("SupportPatrol")));
    AddChildToComposite(Sequence, CreateWanderTask(TreeOuter, 500.0f + Snapshot.SupportScore * 600.0f));
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

UBTTask_DetectPlayer* UPlayerBehaviorTreeBuilder::CreateDetectPlayerTask(UBehaviorTree* TreeOuter, float DetectionRange) const
{
    UBTTask_DetectPlayer* Task = NewObject<UBTTask_DetectPlayer>(TreeOuter);
    Task->DetectionRange = DetectionRange;
    return Task;
}

UBTTask_Wander* UPlayerBehaviorTreeBuilder::CreateWanderTask(UBehaviorTree* TreeOuter, float WanderRadius, float AcceptanceRadius) const
{
    UBTTask_Wander* Task = NewObject<UBTTask_Wander>(TreeOuter);
    Task->WanderRadius = WanderRadius;
    Task->AcceptanceRadius = AcceptanceRadius;
    return Task;
}
