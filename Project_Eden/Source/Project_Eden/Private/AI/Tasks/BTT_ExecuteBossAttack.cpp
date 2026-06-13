#include "AI/Tasks/BTT_ExecuteBossAttack.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AI/Controllers/EnemyAIController.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/BossAttackPatternSelector.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "Abilities/GameplayAbility.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "GameplayTags/GP_Tags.h"

namespace BossAttackTask
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	bool GetBool(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) && BlackboardComponent->GetValueAsBool(KeyName);
	}

	float GetFloat(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, float DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsFloat(KeyName) : DefaultValue;
	}

	int32 GetInt(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, int32 DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsInt(KeyName) : DefaultValue;
	}

	FName GetName(const UBlackboardComponent* BlackboardComponent, const FName& KeyName, FName DefaultValue)
	{
		return HasBlackboardKey(BlackboardComponent, KeyName) ? BlackboardComponent->GetValueAsName(KeyName) : DefaultValue;
	}

	FGPBossAttackPatternContext BuildPatternContext(
		const APawn* ControlledPawn,
		const UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag)
	{
		FGPBossAttackPatternContext Context;
		Context.DefaultAttackAbilityTag = DefaultAttackAbilityTag;

		if (!IsValid(BlackboardComponent))
		{
			return Context;
		}

		const AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
		const float BlackboardDistance = GetFloat(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget, 0.0f);
		Context.DistanceToTarget = IsValid(ControlledPawn) && IsValid(TargetActor)
			? FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation())
			: BlackboardDistance;

		Context.Aggression = GetFloat(BlackboardComponent, EnemyBlackboardKeys::Aggression, Context.Aggression);
		Context.PreferredRange = GetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredRange, Context.PreferredRange);
		Context.HealthRatio = GetFloat(BlackboardComponent, EnemyBlackboardKeys::HealthRatio, Context.HealthRatio);
		Context.BossPhase = GetInt(BlackboardComponent, EnemyBlackboardKeys::BossPhase, Context.BossPhase);
		Context.EnemyMode = GetName(BlackboardComponent, EnemyBlackboardKeys::EnemyMode, Context.EnemyMode);
		Context.FocusTargetRule = GetName(BlackboardComponent, EnemyBlackboardKeys::FocusTargetRule, Context.FocusTargetRule);
		Context.bCanUseBossHeavyAttack = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossHeavyAttack);
		Context.bCanUseBossSweepAttack = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossSweepAttack);
		Context.bCanUseBossAreaAttack = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBossAreaAttack);
		Context.bCanSummonAdds = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanSummonAdds);
		Context.ChainBreakCount = GetInt(BlackboardComponent, EnemyBlackboardKeys::ChainBreakCount, Context.ChainBreakCount);
		Context.bIsGroggy = GetBool(BlackboardComponent, EnemyBlackboardKeys::bIsGroggy);
		Context.bCanUseBullPattern = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseBullPattern);
		Context.bBullPatternActive = GetBool(BlackboardComponent, EnemyBlackboardKeys::bBullPatternActive);
		Context.bShouldTeleport = GetBool(BlackboardComponent, EnemyBlackboardKeys::bShouldTeleport);
		Context.PreferredHoverHeight = GetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredHoverHeight, Context.PreferredHoverHeight);
		Context.PreferredAirRange = GetFloat(BlackboardComponent, EnemyBlackboardKeys::PreferredAirRange, Context.PreferredAirRange);
		Context.WingCoreBreakCount = GetInt(BlackboardComponent, EnemyBlackboardKeys::WingCoreBreakCount, Context.WingCoreBreakCount);
		Context.bCanExposeWingCore = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanExposeWingCore);
		Context.bWingCoreExposed = GetBool(BlackboardComponent, EnemyBlackboardKeys::bWingCoreExposed);
		Context.bCanUseLaserPattern = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUseLaserPattern);
		Context.bCanUsePrismPattern = GetBool(BlackboardComponent, EnemyBlackboardKeys::bCanUsePrismPattern);
		Context.bCrystalPrismActive = HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::CrystalPrismActor)
			&& IsValid(BlackboardComponent->GetValueAsObject(EnemyBlackboardKeys::CrystalPrismActor));

		if (const UGP_MatadorBossStateComponent* MatadorStateComponent = IsValid(ControlledPawn) ? ControlledPawn->FindComponentByClass<UGP_MatadorBossStateComponent>() : nullptr)
		{
			// The replicated state component is the authority for chain/groggy state; Blackboard remains a readable snapshot.
			Context.ChainBreakCount = MatadorStateComponent->GetChainBreakCount();
			Context.ChainBreakTarget = MatadorStateComponent->GetChainBreakTarget();
			Context.bIsGroggy = MatadorStateComponent->IsGroggy();
			Context.bBullPatternActive = IsValid(MatadorStateComponent->GetActiveBullActor());
		}

		if (const UGP_CrystalSeraphStateComponent* CrystalStateComponent = IsValid(ControlledPawn) ? ControlledPawn->FindComponentByClass<UGP_CrystalSeraphStateComponent>() : nullptr)
		{
			// The component owns the real mechanic state; Blackboard values remain optional BT-readable mirrors.
			Context.bIsCrystalSeraph = true;
			Context.WingCoreBreakCount = CrystalStateComponent->GetWingCoreBreakCount();
			Context.WingCoreBreakTarget = CrystalStateComponent->GetWingCoreBreakTarget();
			Context.bIsGroggy = CrystalStateComponent->IsGroggy();
			Context.bWingCoreExposed = CrystalStateComponent->IsWingCoreExposed();
			Context.bCrystalPrismActive = IsValid(CrystalStateComponent->GetCrystalPrismActor());
		}

		return Context;
	}

	bool HasGrantedAbilityForTag(const UAbilitySystemComponent* ASC, const FGameplayTag& AbilityTag)
	{
		if (!IsValid(ASC) || !AbilityTag.IsValid())
		{
			return false;
		}

		for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
		{
			const UGameplayAbility* Ability = AbilitySpec.Ability;
			if (IsValid(Ability) && Ability->GetAssetTags().HasTagExact(AbilityTag))
			{
				return true;
			}
		}

		return false;
	}

	bool TryActivateAbilityByTag(UAbilitySystemComponent* ASC, const FGameplayTag& AbilityTag)
	{
		if (!IsValid(ASC) || !AbilityTag.IsValid())
		{
			return false;
		}

		if (UGP_AbilitySystemComponent* GPASC = Cast<UGP_AbilitySystemComponent>(ASC))
		{
			return GPASC->TryActivateAbilityByTag(AbilityTag);
		}

		return ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
	}
}

UBTT_ExecuteBossAttack::UBTT_ExecuteBossAttack()
{
	// The BT owns the attack entry, while this node re-scores the pattern from live evaluation blackboard values every attack.
	NodeName = TEXT("Select And Execute Boss Attack");
	BossSweepAttackChance = 0.0f;
}

EBTNodeResult::Type UBTT_ExecuteBossAttack::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = nullptr;
	APawn* ControlledPawn = nullptr;
	UBlackboardComponent* BlackboardComponent = nullptr;
	if (!EnemyBTTaskCommon::TryGetTaskContext(OwnerComp, AIController, ControlledPawn, BlackboardComponent))
	{
		return EBTNodeResult::Failed;
	}

	if (AEnemyAIController* EnemyAIController = Cast<AEnemyAIController>(AIController))
	{
		// Re-score the player target from the latest AI evaluation before choosing a boss pattern.
		EnemyAIController->RequestTargetActorReevaluation();
	}

	AActor* TargetActor = EnemyBTTaskCommon::GetTargetActor(BlackboardComponent);
	if (IsValid(TargetActor) && BossAttackTask::HasBlackboardKey(BlackboardComponent, EnemyBlackboardKeys::DistanceToTarget))
	{
		BlackboardComponent->SetValueAsFloat(EnemyBlackboardKeys::DistanceToTarget, FVector::Dist2D(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation()));
	}

	if (bStopMovementBeforeAttack)
	{
		AIController->StopMovement();
	}

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
		UE_LOG(LogEnemyAI, Warning, TEXT("[BossAI] Cannot execute boss pattern - missing ASC: %s"), *GetNameSafe(ControlledPawn));
		return EBTNodeResult::Failed;
	}

	const FGPBossAttackPatternContext PatternContext = BossAttackTask::BuildPatternContext(ControlledPawn, BlackboardComponent, AttackAbilityTag);
	const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(PatternContext);
	for (const FGPBossAttackPatternCandidate& Candidate : Candidates)
	{
		if (!BossAttackTask::HasGrantedAbilityForTag(ASC, Candidate.AbilityTag))
		{
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[BossAI] Pattern skipped because ability is not granted: %s Tag=%s"),
				*Candidate.DebugName.ToString(),
				*Candidate.AbilityTag.ToString());
			continue;
		}

		const bool bActivated = BossAttackTask::TryActivateAbilityByTag(ASC, Candidate.AbilityTag);
		if (bActivated)
		{
			UE_LOG(
				LogEnemyAI,
				Log,
				TEXT("[BossAI] Pattern selected from evaluation: %s Score=%.2f Tag=%s Activated=1 Target=%s Candidates=%s"),
				*Candidate.DebugName.ToString(),
				Candidate.Score,
				*Candidate.AbilityTag.ToString(),
				*EnemyAIDebugUtils::DescribeActor(TargetActor),
				*FGPBossAttackPatternSelector::DescribeCandidates(Candidates));
			return EBTNodeResult::Succeeded;
		}

		// Activation can fail because of cooldowns, blocked tags, or ability state; try the next scored pattern before failing the task.
		UE_LOG(
			LogEnemyAI,
			Warning,
			TEXT("[BossAI] Pattern activation failed, trying next candidate: %s Score=%.2f Tag=%s Target=%s Candidates=%s"),
			*Candidate.DebugName.ToString(),
			Candidate.Score,
			*Candidate.AbilityTag.ToString(),
			*EnemyAIDebugUtils::DescribeActor(TargetActor),
			*FGPBossAttackPatternSelector::DescribeCandidates(Candidates));
	}

	UE_LOG(
		LogEnemyAI,
		Warning,
		TEXT("[BossAI] No activatable ability matched selected boss patterns: %s Candidates=%s"),
		*GetNameSafe(ControlledPawn),
		*FGPBossAttackPatternSelector::DescribeCandidates(Candidates));
	return EBTNodeResult::Failed;
}
