#include "AI/Tasks/BTT_ExecuteEnemyAttack.h"

#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameplayTags/GP_Tags.h"

namespace BTT_ExecuteEnemyAttack_Internal
{
	FGameplayTag ResolveAttackAbilityTagForContext(
		const APawn* ControlledPawn,
		const UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag,
		float BossSweepAttackChance)
	{
		(void)ControlledPawn;
		(void)BlackboardComponent;
		(void)BossSweepAttackChance;

		// Generic attack tasks now execute their configured tag literally.
		// Boss pattern switching belongs to UBTT_ExecuteBossAttack or explicit boss task nodes in the BT.
		return DefaultAttackAbilityTag;
	}
}

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

	const FGameplayTag EffectiveAttackAbilityTag = BTT_ExecuteEnemyAttack_Internal::ResolveAttackAbilityTagForContext(ControlledPawn, BlackboardComponent, AttackAbilityTag, BossSweepAttackChance);
	if (!EffectiveAttackAbilityTag.IsValid())
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
		UE_LOG(LogTemp, Warning, TEXT("[EnemyAI] Cannot execute attack ability because ASC is missing: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	bool bActivated = false;

	if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
	{
		bActivated = GPASC->TryActivateAbilityByTag(EffectiveAttackAbilityTag);
	}
	else
	{
		// Keep non-project ability system components compatible with the same gameplay tag contract.
		bActivated = ASC->TryActivateAbilitiesByTag(EffectiveAttackAbilityTag.GetSingleTagContainer());
	}

	return bActivated ? EBTNodeResult::Succeeded : EBTNodeResult::Failed;
}

FString UBTT_ExecuteEnemyAttack::GetStaticDescription() const
{
	return FString::Printf(TEXT("Attack tag: %s"), *AttackAbilityTag.ToString());
}
