#include "AI/Tasks/BTT_RunEnemyPositioningEQSQuery.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyEQSNames.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AISystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"

UBTT_RunEnemyPositioningEQSQuery::UBTT_RunEnemyPositioningEQSQuery()
{
	NodeName = TEXT("Run Enemy Positioning EQS Query");

	// EQS 위치 결과는 Vector Blackboard 키에만 기록되도록 제한한다.
	MoveLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, MoveLocationKey));
	QueryFinishedDelegate = FQueryFinishedSignature::CreateUObject(this, &ThisClass::OnQueryFinished);
}

void UBTT_RunEnemyPositioningEQSQuery::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (Asset.BlackboardAsset)
	{
		MoveLocationKey.ResolveSelectedKey(*Asset.BlackboardAsset);
	}
}

EBTNodeResult::Type UBTT_RunEnemyPositioningEQSQuery::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EQS] 실행 실패 - TaskContext 획득 실패"));
		return EBTNodeResult::Failed;
	}

	if (!IsValid(QueryTemplate))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EQS] 실행 실패 - QueryTemplate 미지정: %s"), *GetNameSafe(this));
		return EBTNodeResult::Failed;
	}

	UE_LOG(
		LogEnemyAI,
		Verbose,
		TEXT("[EQS] 실행 시작: Query=%s Owner=%s"),
		*GetNameSafe(QueryTemplate),
		*EnemyAIDebugUtils::DescribeActor(ControlledPawn));

	FEnemyPositioningEQSQueryTaskMemory* TaskMemory = CastInstanceNodeMemory<FEnemyPositioningEQSQueryTaskMemory>(NodeMemory);

	// EQS의 Querier는 적 Pawn으로 두어 기본 Context_Querier가 자연스럽게 동작하게 한다.
	FEnvQueryRequest QueryRequest(QueryTemplate, ControlledPawn);
	if (bInjectBlackboardParameters)
	{
		ApplyNamedParams(QueryRequest, BlackboardComponent);
	}

	TaskMemory->RequestID = QueryRequest.Execute(RunMode, QueryFinishedDelegate);
	if (TaskMemory->RequestID >= 0)
	{
		WaitForMessage(OwnerComp, UBrainComponent::AIMessage_QueryFinished, TaskMemory->RequestID);
		return EBTNodeResult::InProgress;
	}

	UE_LOG(
		LogEnemyAI,
		Warning,
		TEXT("[EQS] 실행 요청 생성 실패: Query=%s Owner=%s"),
		*GetNameSafe(QueryTemplate),
		*EnemyAIDebugUtils::DescribeActor(ControlledPawn));
	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTT_RunEnemyPositioningEQSQuery::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	if (UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(OwnerComp.GetWorld()))
	{
		const FEnemyPositioningEQSQueryTaskMemory* TaskMemory = CastInstanceNodeMemory<FEnemyPositioningEQSQueryTaskMemory>(NodeMemory);
		if (TaskMemory->RequestID >= 0)
		{
			UE_LOG(LogEnemyAI, Verbose, TEXT("[EQS] 요청 중단: Query=%s RequestID=%d"), *GetNameSafe(QueryTemplate), TaskMemory->RequestID);
			QueryManager->AbortQuery(TaskMemory->RequestID);
		}
	}

	return EBTNodeResult::Aborted;
}

uint16 UBTT_RunEnemyPositioningEQSQuery::GetInstanceMemorySize() const
{
	return sizeof(FEnemyPositioningEQSQueryTaskMemory);
}

void UBTT_RunEnemyPositioningEQSQuery::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryInit::Type InitType) const
{
	InitializeNodeMemory<FEnemyPositioningEQSQueryTaskMemory>(NodeMemory, InitType);
}

void UBTT_RunEnemyPositioningEQSQuery::CleanupMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTMemoryClear::Type CleanupType) const
{
	CleanupNodeMemory<FEnemyPositioningEQSQueryTaskMemory>(NodeMemory, CleanupType);
}

FString UBTT_RunEnemyPositioningEQSQuery::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("%s\nQuery: %s\nResult Key: %s"),
		*Super::GetStaticDescription(),
		*GetNameSafe(QueryTemplate),
		*MoveLocationKey.SelectedKeyName.ToString());
}

void UBTT_RunEnemyPositioningEQSQuery::ApplyNamedParams(FEnvQueryRequest& QueryRequest, const UBlackboardComponent* BlackboardComponent) const
{
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	// 공용 Blackboard 값을 named param으로 넘겨 같은 EQS 에셋을 여러 적이 재사용하게 만든다.
	QueryRequest
		.SetFloatParam(
			EnemyEQSNames::PreferredRangeParam,
			EnemyBTTaskCommon::GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, 600.0f, 0.0f, 3000.0f))
		.SetFloatParam(
			EnemyEQSNames::CoverPreferenceParam,
			EnemyBTTaskCommon::GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::CoverPreference, 0.35f, 0.0f, 1.0f))
		.SetFloatParam(
			EnemyEQSNames::AggressionParam,
			EnemyBTTaskCommon::GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::Aggression, 0.5f, 0.0f, 1.0f))
		.SetFloatParam(
			EnemyEQSNames::RetreatThresholdParam,
			EnemyBTTaskCommon::GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::RetreatThreshold, 0.35f, 0.0f, 1.0f));
}

void UBTT_RunEnemyPositioningEQSQuery::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (!Result.IsValid())
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EQS] 완료 실패 - Result가 유효하지 않습니다: Query=%s"), *GetNameSafe(QueryTemplate));
		return;
	}

	if (Result->IsAborted())
	{
		UE_LOG(LogEnemyAI, Verbose, TEXT("[EQS] 요청이 중단되었습니다: Query=%s"), *GetNameSafe(QueryTemplate));
		return;
	}

	AActor* QueryOwner = Cast<AActor>(Result->Owner.Get());
	if (APawn* QueryOwnerPawn = Cast<APawn>(QueryOwner))
	{
		QueryOwner = QueryOwnerPawn->GetController();
	}

	UBehaviorTreeComponent* BehaviorTreeComponent = QueryOwner != nullptr
		? QueryOwner->FindComponentByClass<UBehaviorTreeComponent>()
		: nullptr;

	if (!IsValid(BehaviorTreeComponent))
	{
		UE_LOG(LogEnemyAI, Warning, TEXT("[EQS] 완료 실패 - BehaviorTreeComponent를 찾지 못했습니다: Query=%s"), *GetNameSafe(QueryTemplate));
		return;
	}

	UBlackboardComponent* BlackboardComponent = BehaviorTreeComponent->GetBlackboardComponent();
	const bool bSuccess = Result->IsSuccessful() && (Result->Items.Num() > 0) && IsValid(BlackboardComponent);

	if (bSuccess)
	{
		const FVector ResultLocation = Result->GetItemAsLocation(0);

		// Actor 결과든 Point 결과든 첫 번째 최적 후보의 위치만 MoveToLocation에 기록한다.
		BlackboardComponent->SetValueAsVector(MoveLocationKey.SelectedKeyName, ResultLocation);

		UE_LOG(
			LogEnemyAI,
			Log,
			TEXT("[EQS] 성공: Query=%s Owner=%s Result=%s"),
			*GetNameSafe(QueryTemplate),
			*EnemyAIDebugUtils::DescribeActor(QueryOwner),
			*EnemyAIDebugUtils::DescribeLocation(ResultLocation));
	}
	else
	{
		if (bClearMoveLocationOnFail && IsValid(BlackboardComponent))
		{
			BlackboardComponent->ClearValue(MoveLocationKey.SelectedKeyName);
		}

		UE_LOG(
			LogEnemyAI,
			Warning,
			TEXT("[EQS] 실패: Query=%s Owner=%s ClearOnFail=%s"),
			*GetNameSafe(QueryTemplate),
			*EnemyAIDebugUtils::DescribeActor(QueryOwner),
			bClearMoveLocationOnFail ? TEXT("true") : TEXT("false"));
	}

	FAIMessage::Send(
		BehaviorTreeComponent,
		FAIMessage(UBrainComponent::AIMessage_QueryFinished, this, Result->QueryID, bSuccess));
}
