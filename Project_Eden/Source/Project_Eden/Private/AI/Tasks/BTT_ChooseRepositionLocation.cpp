#include "AI/Tasks/BTT_ChooseRepositionLocation.h"

#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"

UBTT_ChooseRepositionLocation::UBTT_ChooseRepositionLocation()
{
	NodeName = TEXT("Choose Reposition Location");
}

EBTNodeResult::Type UBTT_ChooseRepositionLocation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
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
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAI] 재배치 위치 계산을 건너뜁니다. TargetActor가 없습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	const float PreferredRange = FMath::Max(
		MinPreferredRange,
		EnemyBTTaskCommon::GetClampedBlackboardFloat(
			BlackboardComponent,
			EnemyBlackboardKeys::PreferredRange,
			600.0f,
			0.0f,
			3000.0f));

	const float Aggression = EnemyBTTaskCommon::GetClampedBlackboardFloat(
		BlackboardComponent,
		EnemyBlackboardKeys::Aggression,
		0.5f,
		0.0f,
		1.0f);

	const float CoverPreference = EnemyBTTaskCommon::GetClampedBlackboardFloat(
		BlackboardComponent,
		EnemyBlackboardKeys::CoverPreference,
		0.35f,
		0.0f,
		1.0f);

	FVector FromTargetDirection = ControlledPawn->GetActorLocation() - TargetActor->GetActorLocation();
	FromTargetDirection.Z = 0.0f;
	if (!FromTargetDirection.Normalize())
	{
		FromTargetDirection = ControlledPawn->GetActorForwardVector().GetSafeNormal2D();
	}

	FVector SideDirection = FVector::CrossProduct(FVector::UpVector, FromTargetDirection).GetSafeNormal();
	if (SideDirection.IsNearlyZero())
	{
		SideDirection = ControlledPawn->GetActorRightVector().GetSafeNormal2D();
	}

	const float DesiredRange = FMath::Lerp(PreferredRange * 1.15f, PreferredRange * 0.85f, Aggression);
	const float LateralOffset = FMath::Lerp(MinLateralOffset, MaxLateralOffset, CoverPreference);
	const float SideSign = FMath::RandBool() ? 1.0f : -1.0f;

	const FVector DesiredLocation =
		TargetActor->GetActorLocation()
		+ FromTargetDirection * DesiredRange
		+ SideDirection * (LateralOffset * SideSign);

	FVector RepositionLocation = FVector::ZeroVector;
	const bool bFoundLocation = EnemyBTTaskCommon::TryResolveReachableLocation(
		ControlledPawn->GetWorld(),
		DesiredLocation,
		SearchRadius,
		RepositionLocation);

	if (!bFoundLocation)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[EnemyAI] 재배치 위치를 찾지 못했습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	BlackboardComponent->SetValueAsVector(EnemyBlackboardKeys::MoveToLocation, RepositionLocation);
	return EBTNodeResult::Succeeded;
}

FString UBTT_ChooseRepositionLocation::GetStaticDescription() const
{
	return FString::Printf(
		TEXT("최소 선호 거리: %.0f, 측면 오프셋: %.0f~%.0f"),
		MinPreferredRange,
		MinLateralOffset,
		MaxLateralOffset);
}
