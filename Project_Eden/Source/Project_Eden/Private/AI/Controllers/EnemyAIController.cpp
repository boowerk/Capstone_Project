#include "AI/Controllers/EnemyAIController.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyLLMEvaluation.h"
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

bool AEnemyAIController::ApplyEnemyEvaluationToBlackboard(const FEnemyLLMEvaluation& InEvaluation)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Blackboard가 없어 평가 데이터를 적용할 수 없습니다: %s"), *GetName());
		return false;
	}

	FEnemyLLMEvaluation SafeEvaluation = InEvaluation;
	SafeEvaluation.ValidateAndClamp();

	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::EnemyMode, SafeEvaluation.GetEnemyModeBlackboardValue());
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::Aggression, SafeEvaluation.Aggression);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::PreferredRange, SafeEvaluation.PreferredRange);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::RetreatThreshold, SafeEvaluation.RetreatThreshold);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::ChasePersistence, SafeEvaluation.ChasePersistence);
	BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::CoverPreference, SafeEvaluation.CoverPreference);
	BlackboardComponent->SetValueAsName(EnemyBlackboardKeys::FocusTargetRule, SafeEvaluation.GetFocusTargetRuleBlackboardValue());

	return true;
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
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent) || !IsValid(InPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAIController] Blackboard 초기값 설정을 건너뜁니다. Controller=%s Pawn=%s"), *GetName(), *GetNameSafe(InPawn));
		return;
	}

	// 런타임 상태 키는 컨트롤러가 초기화하고, 이후 서비스/태스크가 같은 키를 공유해서 갱신한다.
	BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, InPawn->GetActorLocation());

	const FEnemyLLMEvaluation DefaultEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	ApplyEnemyEvaluationToBlackboard(DefaultEvaluation);
}

UBlackboardComponent* AEnemyAIController::GetEnemyBlackboardComponent()
{
	return GetBlackboardComponent();
}
