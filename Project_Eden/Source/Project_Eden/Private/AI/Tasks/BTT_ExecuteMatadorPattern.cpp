#include "AI/Tasks/BTT_ExecuteMatadorPattern.h"

#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AIController.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BlackboardData.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Characters/GP_MatadorMageBossCharacter.h"
#include "Kismet/GameplayStatics.h"

namespace MatadorPatternTask
{
	void SetOptionalBlackboardBool(UBlackboardComponent* BlackboardComponent, const FName& KeyName, bool bValue)
	{
		if (IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey)
		{
			BlackboardComponent->SetValueAsBool(KeyName, bValue);
		}
	}
}

UBTT_ExecuteMatadorPattern::UBTT_ExecuteMatadorPattern()
{
	NodeName = TEXT("Execute Matador Pattern");
}

EBTNodeResult::Type UBTT_ExecuteMatadorPattern::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	AGP_MatadorMageBossCharacter* MatadorBoss = Cast<AGP_MatadorMageBossCharacter>(ControlledPawn);
	UGP_MatadorBossStateComponent* MatadorStateComponent = IsValid(MatadorBoss) ? MatadorBoss->GetMatadorStateComponent() : nullptr;
	if (!IsValid(MatadorBoss) || !IsValid(MatadorStateComponent))
	{
		return EBTNodeResult::Failed;
	}

	if (bEnterGroggyWhenChainComplete && !MatadorStateComponent->IsGroggy() && MatadorStateComponent->GetChainBreakCount() >= MatadorStateComponent->GetChainBreakTarget())
	{
		MatadorBoss->RequestEnterGroggy();
		UE_LOG(LogEnemyAI, Log, TEXT("[MatadorAI] Enter groggy requested by BT. Boss=%s"), *GetNameSafe(MatadorBoss));
		return EBTNodeResult::Succeeded;
	}

	if (MatadorStateComponent->IsGroggy())
	{
		return EBTNodeResult::Failed;
	}

	if (IsValid(MatadorStateComponent->GetActiveBullActor()))
	{
		return bSucceedWhenBullAlreadyActive ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (!IsValid(TargetActor) && bFallbackToPlayerPawn)
	{
		TargetActor = UGameplayStatics::GetPlayerPawn(MatadorBoss, 0);
	}
	if (!IsValid(TargetActor))
	{
		return EBTNodeResult::Failed;
	}

	const float DistanceToTarget = FVector::Dist2D(MatadorBoss->GetActorLocation(), TargetActor->GetActorLocation());
	if (DistanceToTarget < BullPatternMinRange || DistanceToTarget > BullPatternMaxRange)
	{
		UE_LOG(
			LogEnemyAI,
			Verbose,
			TEXT("[MatadorAI] Bull pattern skipped by range. Boss=%s Target=%s Distance=%.0f"),
			*GetNameSafe(MatadorBoss),
			*EnemyAIDebugUtils::DescribeActor(TargetActor),
			DistanceToTarget);
		return EBTNodeResult::Failed;
	}

	if (bStopMovementBeforePattern)
	{
		AIController->StopMovement();
	}

	if (bFaceTargetBeforePattern)
	{
		FVector LookDirection = TargetActor->GetActorLocation() - MatadorBoss->GetActorLocation();
		LookDirection.Z = 0.0f;
		if (!LookDirection.IsNearlyZero())
		{
			MatadorBoss->SetActorRotation(LookDirection.Rotation());
		}
	}

	const bool bStarted = MatadorBoss->RequestStartBullPattern(TargetActor);
	if (bStarted)
	{
		MatadorPatternTask::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBullPattern, false);
		MatadorPatternTask::SetOptionalBlackboardBool(BlackboardComponent, EnemyBlackboardKeys::bBullPatternActive, true);
	}

	UE_LOG(
		LogEnemyAI,
		Log,
		TEXT("[MatadorAI] Bull pattern requested by BT. Boss=%s Target=%s Distance=%.0f Started=%d"),
		*GetNameSafe(MatadorBoss),
		*EnemyAIDebugUtils::DescribeActor(TargetActor),
		DistanceToTarget,
		bStarted ? 1 : 0);

	return bStarted ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTT_ExecuteMatadorPattern::GetStaticDescription() const
{
	return FString::Printf(TEXT("Bull range %.0f-%.0f"), BullPatternMinRange, BullPatternMaxRange);
}
