#include "AI/Controllers/EnemyAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Pawn.h"

AEnemyAIController::AEnemyAIController()
{
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	InitializeBehaviorTree(InPawn);
}

bool AEnemyAIController::InitializeBehaviorTree(APawn* InPawn)
{
	if (!IsValid(InPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Possess된 Pawn이 유효하지 않습니다."));
		return false;
	}

	if (!IsValid(DefaultBehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAIController] Behavior Tree 에셋이 아직 지정되지 않았습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	UBlackboardData* BlackboardAssetToUse = DefaultBlackboardAsset;
	if (!IsValid(BlackboardAssetToUse))
	{
		BlackboardAssetToUse = DefaultBehaviorTreeAsset->BlackboardAsset;
	}

	if (!IsValid(BlackboardAssetToUse))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Blackboard 에셋이 없습니다: %s"), *GetNameSafe(DefaultBehaviorTreeAsset));
		return false;
	}

	UBlackboardComponent* BlackboardComponent = nullptr;

	// 공용 Blackboard 에셋이 준비되면 여기서 UseBlackboard를 호출해 키 기본값을 초기화한다.
	if (!UseBlackboard(BlackboardAssetToUse, BlackboardComponent) || !IsValid(BlackboardComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Blackboard 초기화에 실패했습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	InitializeBlackboardValues(InPawn);

	// 공용 Behavior Tree 에셋이 준비되면 여기서 RunBehaviorTree를 호출해 적 AI 루프를 시작한다.
	if (!RunBehaviorTree(DefaultBehaviorTreeAsset))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Behavior Tree 실행에 실패했습니다: %s"), *GetNameSafe(InPawn));
		return false;
	}

	return true;
}

void AEnemyAIController::InitializeBlackboardValues(APawn* InPawn)
{
	// 다음 단계에서 HomeLocation, TargetActor 같은 공용 Blackboard 키를 여기서 세팅한다.
}
