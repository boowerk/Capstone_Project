#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"

#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "GameplayTags/GP_Tags.h"

UBTT_ExecuteEnemyAttack::UBTT_ExecuteEnemyAttack()
{
	NodeName = TEXT("Execute Enemy Attack");
	AttackAbilityTag = GPTags::Ability::Enemy::Attack_Melee;
}

EBTNodeResult::Type UBTT_ExecuteEnemyAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	if (!AttackAbilityTag.IsValid())
	{
		return EBTNodeResult::Failed;
	}

	if (bStopMovementBeforeAttack)
	{
		AIController->StopMovement();
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (bFaceTargetBeforeAttack && IsValid(TargetActor))
	{
		FVector LookDirection = TargetActor->GetActorLocation() - ControlledPawn->GetActorLocation();
		LookDirection.Z = 0.0f;
		if (!LookDirection.IsNearlyZero())
		{
			ControlledPawn->SetActorRotation(LookDirection.Rotation());
		}
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!IsValid(ASC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] 공격 어빌리티를 실행할 ASC가 없습니다: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	bool bActivated = false;

	if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
	{
		bActivated = GPASC->TryActivateAbilityByTag(AttackAbilityTag);
	}
	else
	{
		// 프로젝트 ASC가 아니더라도 태그 계약만 맞으면 실행 가능하도록 폴백을 둔다.
		bActivated = ASC->TryActivateAbilitiesByTag(AttackAbilityTag.GetSingleTagContainer());
	}

	return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTT_ExecuteEnemyAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("공격 태그: %s"), *AttackAbilityTag.ToString());
}
