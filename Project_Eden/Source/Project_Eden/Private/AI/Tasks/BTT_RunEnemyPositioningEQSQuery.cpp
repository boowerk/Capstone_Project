#include "AI/Tasks/BTT_RunEnemyPositioningEQSQuery.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Data/EnemyEQSNames.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AISystem.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BrainComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "NavigationSystem.h"

UBTT_RunEnemyPositioningEQSQuery::UBTT_RunEnemyPositioningEQSQuery()
{
	NodeName = TEXT("Run Enemy Positioning EQS Query");

	// EQS 위치 결과는 Vector Blackboard 키에만 기록되도록 제한한다.
	MoveLocationKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(ThisClass, MoveLocationKey));
	// Default to the shared movement key so patrol can still work if a BT node is added before designers set details.
	MoveLocationKey.SelectedKeyName = EnemyBlackboardKeys::MoveToLocation;
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

	const bool bShouldReturnHome = BlackboardComponent->GetKeyID(EnemyBlackboardKeys::bShouldReturnHome) != FBlackboard::InvalidKey
		&& BlackboardComponent->GetValueAsBool(EnemyBlackboardKeys::bShouldReturnHome);
	if (ShouldYieldToReturnHome(bShouldReturnHome))
	{
		// Patrol is the first no-target branch in Boss_Common; fail it explicitly so the selector can reach ReturnHome.
		UE_LOG(LogEnemyAI, Verbose, TEXT("[Patrol] Yielding to ReturnHome: Pawn=%s"), *EnemyAIDebugUtils::DescribeActor(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	if (!IsValid(QueryTemplate))
	{
		if (TrySetNavigationFallbackLocation(BlackboardComponent, ControlledPawn))
		{
			// Missing patrol EQS should not leave an idle enemy frozen at its spawn point.
			return EBTNodeResult::Succeeded;
		}

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
		ApplyNamedParams(QueryRequest, BlackboardComponent, ControlledPawn);
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

	if (TrySetNavigationFallbackLocation(BlackboardComponent, ControlledPawn))
	{
		// A failed request creation is recoverable for patrol movement when NavMesh can provide a point.
		return EBTNodeResult::Succeeded;
	}

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

void UBTT_RunEnemyPositioningEQSQuery::ApplyNamedParams(FEnvQueryRequest& QueryRequest, const UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const
{
	if (!IsValid(BlackboardComponent))
	{
		return;
	}

	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	const float PatrolRadius = IsValid(EnemyCharacter) ? EnemyCharacter->GetPatrolRadius() : 1200.0f;

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
			EnemyBTTaskCommon::GetClampedBlackboardFloat(BlackboardComponent, EnemyBlackboardKeys::RetreatThreshold, 0.35f, 0.0f, 1.0f))
		// Patrol EQS_PatrolLocation should bind GridHalfSize or CircleRadius to this named param.
		.SetFloatParam(EnemyEQSNames::PatrolRadiusParam, PatrolRadius);
}

FName UBTT_RunEnemyPositioningEQSQuery::GetMoveLocationKeyName() const
{
	return MoveLocationKey.SelectedKeyName != NAME_None
		? MoveLocationKey.SelectedKeyName
		: EnemyBlackboardKeys::MoveToLocation;
}

bool UBTT_RunEnemyPositioningEQSQuery::IsPatrolQuery() const
{
	// Keep this native guard tied to the query role without requiring a risky binary BT asset rewrite.
	return IsValid(QueryTemplate) && QueryTemplate->GetName().Contains(TEXT("Patrol"), ESearchCase::IgnoreCase);
}

bool UBTT_RunEnemyPositioningEQSQuery::ShouldYieldToReturnHome(bool bShouldReturnHome) const
{
	return bShouldReturnHome && IsPatrolQuery();
}

bool UBTT_RunEnemyPositioningEQSQuery::TrySetNavigationFallbackLocation(UBlackboardComponent* BlackboardComponent, const APawn* ControlledPawn) const
{
	if (!bUseNavigationFallbackOnFail || !IsValid(BlackboardComponent) || !IsValid(ControlledPawn))
	{
		return false;
	}

	UWorld* World = ControlledPawn->GetWorld();
	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!IsValid(NavSystem))
	{
		return false;
	}

	const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn);
	const FVector HomeLocation = EnemyBTTaskCommon::GetBehaviorAnchorLocation(ControlledPawn);
	const float PatrolRadius = IsValid(EnemyCharacter) ? EnemyCharacter->GetPatrolRadius() : 1200.0f;
	const float RequiredMoveDistance = FMath::Clamp(MinimumMoveDistance, 0.0f, PatrolRadius);

	FNavLocation CandidateLocation;
	for (int32 AttemptIndex = 0; AttemptIndex < 8; ++AttemptIndex)
	{
		if (NavSystem->GetRandomReachablePointInRadius(HomeLocation, PatrolRadius, CandidateLocation)
			&& FVector::DistSquared2D(ControlledPawn->GetActorLocation(), CandidateLocation.Location) >= FMath::Square(RequiredMoveDistance))
		{
			BlackboardComponent->SetValueAsVector(GetMoveLocationKeyName(), CandidateLocation.Location);
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[Patrol] Fallback move location selected: Pawn=%s Location=%s"),
				*EnemyAIDebugUtils::DescribeActor(ControlledPawn),
				*EnemyAIDebugUtils::DescribeLocation(CandidateLocation.Location));
			return true;
		}
	}

	if (NavSystem->ProjectPointToNavigation(HomeLocation, CandidateLocation))
	{
		// Last resort keeps MoveToLocation valid; designers should still ensure PatrolRadius covers enough NavMesh.
		BlackboardComponent->SetValueAsVector(GetMoveLocationKeyName(), CandidateLocation.Location);
		UE_LOG(
			LogEnemyAI,
			Warning,
			TEXT("[Patrol] Fallback used home location because no distant patrol point was reachable: Pawn=%s Location=%s"),
			*EnemyAIDebugUtils::DescribeActor(ControlledPawn),
			*EnemyAIDebugUtils::DescribeLocation(CandidateLocation.Location));
		return true;
	}

	return false;
}

bool UBTT_RunEnemyPositioningEQSQuery::IsMoveLocationFarEnough(const APawn* ControlledPawn, const FVector& MoveLocation) const
{
	return IsValid(ControlledPawn)
		&& FVector::DistSquared2D(ControlledPawn->GetActorLocation(), MoveLocation) >= FMath::Square(MinimumMoveDistance);
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
	APawn* QueryOwnerPawn = Cast<APawn>(QueryOwner);
	if (QueryOwnerPawn != nullptr)
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
		if (!IsMoveLocationFarEnough(QueryOwnerPawn, ResultLocation) && TrySetNavigationFallbackLocation(BlackboardComponent, QueryOwnerPawn))
		{
			// Near-zero EQS results look like no patrol, so replace them with a reachable point farther away.
			FAIMessage::Send(
				BehaviorTreeComponent,
				FAIMessage(UBrainComponent::AIMessage_QueryFinished, this, Result->QueryID, true));
			return;
		}

		// Actor 결과든 Point 결과든 첫 번째 최적 후보의 위치만 MoveToLocation에 기록한다.
		BlackboardComponent->SetValueAsVector(GetMoveLocationKeyName(), ResultLocation);

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
		if (TrySetNavigationFallbackLocation(BlackboardComponent, QueryOwnerPawn))
		{
			FAIMessage::Send(
				BehaviorTreeComponent,
				FAIMessage(UBrainComponent::AIMessage_QueryFinished, this, Result->QueryID, true));
			return;
		}

		if (bClearMoveLocationOnFail && IsValid(BlackboardComponent))
		{
			BlackboardComponent->ClearValue(GetMoveLocationKeyName());
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
