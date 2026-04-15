#include "AI/Tasks/BTT_ChooseRetreatLocation.h"

#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"

UBTT_ChooseRetreatLocation::UBTT_ChooseRetreatLocation()
{
	NodeName = TEXT("Choose Retreat Location");
}

EBTNodeResult::Type UBTT_ChooseRetreatLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (!IsValid(TargetActor))
	{
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAI] 후퇴 위치 계산을 건너뜁니다. TargetActor가 없습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	const float PreferredRange = EnemyBTTaskCommon::GetClampedBlackboardFloat(
		BlackboardComponent,
		EnemyBlackboardKeys::PreferredRange,
		600.0f,
		0.0f,
		3000.0f);

	const float CoverPreference = EnemyBTTaskCommon::GetClampedBlackboardFloat(
		BlackboardComponent,
		EnemyBlackboardKeys::CoverPreference,
		0.35f,
		0.0f,
		1.0f);

	FVector AwayDirection = ControlledPawn->GetActorLocation() - TargetActor->GetActorLocation();
	AwayDirection.Z = 0.0f;
	if (!AwayDirection.Normalize())
	{
		AwayDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	const float RetreatDistance = FMath::Max(
		MinRetreatDistance,
		PreferredRange * PreferredRangeScale + CoverPreference * CoverPreferenceDistanceBonus);

	const FVector DesiredLocation = ControlledPawn->GetActorLocation() + AwayDirection * RetreatDistance;

	FVector RetreatLocation = FVector::ZeroVector;
	const bool bFoundLocation = EnemyBTTaskCommon::TryResolveReachableLocation(
		ControlledPawn->GetWorld(),
		DesiredLocation,
		SearchRadius,
		RetreatLocation);

	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAI] 후퇴 위치를 찾지 못했습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, RetreatLocation);
	return EBTNodeResult::Succeeded;
}

FString UBTT_ChooseRetreatLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("최소 후퇴 거리: %.0f, 선호 거리 배율: %.2f, 엄폐 보너스: %.0f"),
		MinRetreatDistance,
		PreferredRangeScale,
		CoverPreferenceDistanceBonus);
}
