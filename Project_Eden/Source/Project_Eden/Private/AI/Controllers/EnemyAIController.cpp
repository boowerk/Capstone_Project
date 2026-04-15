#include "AI/Controllers/EnemyAIController.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyLLMEvaluation.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"

AEnemyAIController::AEnemyAIController()
{
	EnemyPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EnemyPerceptionComponent"));
	SightSenseConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightSenseConfig"));

	// 공용 컨트롤러가 같은 감지 컴포넌트를 쓰도록 AAIController의 기본 참조도 연결한다.
	SetPerceptionComponent(*EnemyPerceptionComponent);

	// 기본값은 폰 감지로 두고, 필요하면 에디터에서 더 좁은 클래스로 제한할 수 있다.
	TargetActorClass = APawn::StaticClass();

	if (IsValid(EnemyPerceptionComponent))
	{
		EnemyPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::HandleTargetPerceptionUpdated);
	}

	ConfigureSightSense();
}

void AEnemyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 에디터에서 바꾼 감지 파라미터를 실제 런타임 설정에 다시 반영한다.
	ConfigureSightSense();
	InitializeBehaviorTree(InPawn);
	RefreshTargetActorFromPerception();
}

void AEnemyAIController::OnUnPossess()
{
	// 폰을 잃으면 이전 타겟이 남아 있지 않도록 Blackboard를 정리한다.
	SetBlackboardTargetActor(nullptr);

	Super::OnUnPossess();
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

	// 런타임 상태 키는 컨트롤러가 초기화하고, 이후 서비스와 태스크가 같은 키를 공유해서 갱신한다.
	BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, InPawn->GetActorLocation());

	const FEnemyLLMEvaluation DefaultEvaluation = FEnemyLLMEvaluation::MakeSafeDefault();
	ApplyEnemyEvaluationToBlackboard(DefaultEvaluation);
}

UBlackboardComponent* AEnemyAIController::GetEnemyBlackboardComponent()
{
	return GetBlackboardComponent();
}

void AEnemyAIController::ConfigureSightSense()
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(SightSenseConfig))
	{
		return;
	}

	SightSenseConfig->SightRadius = SightRadius;
	SightSenseConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightSenseConfig->PeripheralVisionAngleDegrees = PeripheralVisionAngleDegrees;
	SightSenseConfig->SetMaxAge(SightMaxAge);
	SightSenseConfig->AutoSuccessRangeFromLastSeenLocation = AutoSuccessRangeFromLastSeenLocation;
	SightSenseConfig->DetectionByAffiliation.bDetectEnemies = bDetectEnemies;
	SightSenseConfig->DetectionByAffiliation.bDetectFriendlies = bDetectFriendlies;
	SightSenseConfig->DetectionByAffiliation.bDetectNeutrals = bDetectNeutrals;

	// 시야 센스를 공용 기본 센스로 등록해 여러 적이 같은 방식으로 대상을 감지하게 만든다.
	EnemyPerceptionComponent->ConfigureSense(*SightSenseConfig);
	EnemyPerceptionComponent->SetDominantSense(SightSenseConfig->GetSenseImplementation());
	EnemyPerceptionComponent->RequestStimuliListenerUpdate();
}

void AEnemyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	// 개별 자극 결과에 직접 분기하기보다 현재 시점의 유효 타겟 전체를 다시 평가해 일관성을 유지한다.
	RefreshTargetActorFromPerception();
}

void AEnemyAIController::RefreshTargetActorFromPerception()
{
	SetBlackboardTargetActor(SelectBestTargetActorFromPerception());
}

AActor* AEnemyAIController::SelectBestTargetActorFromPerception() const
{
	if (!IsValid(EnemyPerceptionComponent) || !IsValid(GetPawn()))
	{
		return nullptr;
	}

	TArray<AActor*> PerceivedActors;
	EnemyPerceptionComponent->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	AActor* BestTargetActor = nullptr;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	for (AActor* CandidateActor : PerceivedActors)
	{
		if (!IsValidPerceptionTarget(CandidateActor))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(GetPawn()->GetActorLocation(), CandidateActor->GetActorLocation());
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			BestTargetActor = CandidateActor;
		}
	}

	return BestTargetActor;
}

bool AEnemyAIController::IsValidPerceptionTarget(AActor* CandidateActor) const
{
	if (!IsValid(CandidateActor))
	{
		return false;
	}

	if (CandidateActor == GetPawn())
	{
		return false;
	}

	if (TargetActorClass && !CandidateActor->IsA(TargetActorClass))
	{
		return false;
	}

	if (bTargetOnlyPlayerControlledPawns)
	{
		const APawn* CandidatePawn = Cast<APawn>(CandidateActor);
		if (!IsValid(CandidatePawn) || !CandidatePawn->IsPlayerControlled())
		{
			return false;
		}
	}

	return true;
}

void AEnemyAIController::SetBlackboardTargetActor(AActor* NewTargetActor)
{
	UBlackboardComponent* BlackboardComponent = GetEnemyBlackboardComponent();
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	AActor* CurrentTargetActor = Cast<AActor>(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::TargetActor));
	if (CurrentTargetActor == NewTargetActor)
	{
		return;
	}

	if (IsValid(NewTargetActor))
	{
		// 감지에 성공한 가장 적합한 대상을 공유 Blackboard의 TargetActor 키에 기록한다.
		BlackboardComponent->SetValueAsObject(EnemyBlackboardKeys::TargetActor, NewTargetActor);
	}
	else
	{
		// 현재 감지 중인 유효 타겟이 없으면 Blackboard에서 안전하게 타겟을 비운다.
		BlackboardComponent->ClearValue(EnemyBlackboardKeys::TargetActor);
	}
}
