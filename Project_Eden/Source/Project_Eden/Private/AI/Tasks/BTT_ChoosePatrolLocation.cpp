#include "AI/Tasks/BTT_ChoosePatrolLocation.h"

#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"

UBTT_ChoosePatrolLocation::UBTT_ChoosePatrolLocation()
{
	NodeName = TEXT("Choose Patrol Location");
}

EBTNodeResult::Type UBTT_ChoosePatrolLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	const float PreferredRange = EnemyBTTaskCommon::GetClampedBlackboardFloat(
		BlackboardComponent,
		EnemyBlackboardKeys::PreferredRange,
		600.0f,
		0.0f,
		3000.0f);

	const float PatrolRadius = FMath::Max(
		MinPatrolRadius,
		BasePatrolRadius + PreferredRange * PreferredRangeRadiusScale);

	const FVector AnchorLocation = EnemyBTTaskCommon::GetBehaviorAnchorLocation(ControlledPawn);

	FVector PatrolLocation = FVector::ZeroVector;
	const bool bFoundLocation = EnemyBTTaskCommon::TryResolveReachableLocation(
		ControlledPawn->GetWorld(),
		AnchorLocation + FMath::VRand() * PatrolRadius,
		PatrolRadius,
		PatrolLocation);

	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAI] 순찰 위치를 찾지 못했습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, PatrolLocation);
	return EBTNodeResult::Succeeded;
}

FString UBTT_ChoosePatrolLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("기본 반경: %.0f, 최소 반경: %.0f, 선호 거리 배율: %.2f"),
		BasePatrolRadius,
		MinPatrolRadius,
		PreferredRangeRadiusScale);
}
