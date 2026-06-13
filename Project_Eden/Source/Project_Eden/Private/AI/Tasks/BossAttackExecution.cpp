#include "AI/Tasks/BossAttackExecution.h"

#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AI/Data/EnemyBlackboardKeys.h"
#include "AI/Debug/EnemyAIDebugUtils.h"
#include "AI/Tasks/EnemyBTTaskCommon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Characters/GP_CrystalSeraphStateComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_MatadorBossStateComponent.h"

namespace BossAttackExecution
{
	bool HasBlackboardKey(const UBlackboardComponent* BlackboardComponent, const FName& KeyName)
	{
		return IsValid(BlackboardComponent) && BlackboardComponent->GetKeyID(KeyName) != FBlackboard::InvalidKey;
	}

	namespace
	{
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

	bool ShouldUseBossPatternSelector(const APawn* ControlledPawn, const UBlackboardComponent* BlackboardComponent)
	{
		if (!IsValid(ControlledPawn))
		{
			return false;
		}

		if (const AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(ControlledPawn))
		{
			if (EnemyCharacter->IsBossEnemy())
			{
				return true;
			}
		}

		// Boss state components are authoritative even if a Blueprint forgot to mirror the boss flag.
		return IsValid(ControlledPawn->FindComponentByClass<UGP_MatadorBossStateComponent>())
			|| IsValid(ControlledPawn->FindComponentByClass<UGP_CrystalSeraphStateComponent>());
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
			// The replicated state component owns chain/groggy truth; Blackboard stays a readable mirror for BT/debug.
			Context.ChainBreakCount = MatadorStateComponent->GetChainBreakCount();
			Context.ChainBreakTarget = MatadorStateComponent->GetChainBreakTarget();
			Context.bIsGroggy = MatadorStateComponent->IsGroggy();
			Context.bBullPatternActive = IsValid(MatadorStateComponent->GetActiveBullActor());
		}

		if (const UGP_CrystalSeraphStateComponent* CrystalStateComponent = IsValid(ControlledPawn) ? ControlledPawn->FindComponentByClass<UGP_CrystalSeraphStateComponent>() : nullptr)
		{
			// The component owns Crystal Seraph mechanic state; optional Blackboard keys only mirror designer-readable flags.
			Context.bIsCrystalSeraph = true;
			Context.WingCoreBreakCount = CrystalStateComponent->GetWingCoreBreakCount();
			Context.WingCoreBreakTarget = CrystalStateComponent->GetWingCoreBreakTarget();
			Context.bIsGroggy = CrystalStateComponent->IsGroggy();
			Context.bWingCoreExposed = CrystalStateComponent->IsWingCoreExposed();
			Context.bCrystalPrismActive = IsValid(CrystalStateComponent->GetCrystalPrismActor());
		}

		return Context;
	}

	EBTNodeResult::Type ExecuteBestPattern(
		UAbilitySystemComponent* ASC,
		const APawn* ControlledPawn,
		const UBlackboardComponent* BlackboardComponent,
		const FGameplayTag& DefaultAttackAbilityTag,
		const AActor* TargetActor)
	{
		const FGPBossAttackPatternContext PatternContext = BuildPatternContext(ControlledPawn, BlackboardComponent, DefaultAttackAbilityTag);
		const TArray<FGPBossAttackPatternCandidate> Candidates = FGPBossAttackPatternSelector::BuildCandidates(PatternContext);

		for (const FGPBossAttackPatternCandidate& Candidate : Candidates)
		{
			if (!HasGrantedAbilityForTag(ASC, Candidate.AbilityTag))
			{
				UE_LOG(
					LogEnemyAI,
					Log,
					TEXT("[BossAI] Pattern skipped because ability is not granted: %s Tag=%s"),
					*Candidate.DebugName.ToString(),
					*Candidate.AbilityTag.ToString());
				continue;
			}

			if (TryActivateAbilityByTag(ASC, Candidate.AbilityTag))
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

			// Activation can fail because of cooldowns, blocked tags, or ability state; try the next scored pattern before failing.
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
}
