#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_LifeDrainTarget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "DrawDebugHelpers.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_Skill_LifeDrainTarget::UGP_Skill_LifeDrainTarget()
{
	SelectionMode = EGP_SkillSelectionMode::TargetActor;
	bRequireBlockingHit = true;
	bTargetSelectionRequiresCombatEffect = true;
	MaxTargetRange = 900.0f;
	TargetAimAssistRadius = 220.0f;
	bEndAbilityAfterConfirmedExecute = false;
}

void UGP_Skill_LifeDrainTarget::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DrainTickTimerHandle);
	}

	ActiveDrainTarget = nullptr;
	DrainStartTime = 0.0f;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGP_Skill_LifeDrainTarget::ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	if (!IsValid(TargetData.TargetActor))
	{
		FinishDrain(true);
		return;
	}

	ActiveDrainTarget = TargetData.TargetActor;
	DrainStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	ApplyDrainTick();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DrainTickTimerHandle,
			this,
			&ThisClass::TickDrain,
			FMath::Max(DrainTickInterval, 0.05f),
			true);
	}
}

void UGP_Skill_LifeDrainTarget::TickDrain()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		FinishDrain(true);
		return;
	}

	if (DrainDuration > 0.0f && World->GetTimeSeconds() - DrainStartTime >= DrainDuration)
	{
		FinishDrain(false);
		return;
	}

	if (!IsValid(GetAvatarActorFromActorInfo()) || !IsValid(ActiveDrainTarget))
	{
		FinishDrain(true);
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActiveDrainTarget);
	if (!IsValid(TargetASC)
		|| (TargetASC->HasAttributeSetForAttribute(UGP_AttributeSet::GetHealthAttribute())
			&& TargetASC->GetNumericAttribute(UGP_AttributeSet::GetHealthAttribute()) <= KINDA_SMALL_NUMBER))
	{
		FinishDrain(true);
		return;
	}

	if (CanContinueDrain())
	{
		ApplyDrainTick();
	}
}

bool UGP_Skill_LifeDrainTarget::CanContinueDrain() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar) || !IsValid(ActiveDrainTarget))
	{
		return false;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActiveDrainTarget);
	if (!IsValid(TargetASC))
	{
		return false;
	}

	if (TargetASC->HasAttributeSetForAttribute(UGP_AttributeSet::GetHealthAttribute())
		&& TargetASC->GetNumericAttribute(UGP_AttributeSet::GetHealthAttribute()) <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float MaintainRange = FMath::Max(MaxTargetRange, 1.0f) * FMath::Max(MaintainRangeMultiplier, 1.0f);
	if (FVector::DistSquared(Avatar->GetActorLocation(), ActiveDrainTarget->GetActorLocation()) > FMath::Square(MaintainRange))
	{
		return false;
	}

	return HasSelectionLineOfSight(Avatar, ActiveDrainTarget);
}

void UGP_Skill_LifeDrainTarget::ApplyDrainTick()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!IsValid(Avatar)
		|| !IsValid(ActiveDrainTarget)
		|| !CanContinueDrain()
		|| !UGP_BlueprintLibrary::CanApplyCombatEffect(Avatar, ActiveDrainTarget))
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Avatar);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ActiveDrainTarget);
	if (!IsValid(SourceASC) || !IsValid(TargetASC))
	{
		return;
	}

	TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = DamageEffectClass;
	if (!ResolvedDamageEffectClass)
	{
		ResolvedDamageEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage.GE_PrimaryDamage_C"));
	}

	if (ResolvedDamageEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(Avatar, Avatar);

		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(ResolvedDamageEffectClass, GetAbilityLevel(), ContextHandle);
		if (SpecHandle.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, DrainDamagePerTick);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, 0.0f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	const float HealAmount = DrainDamagePerTick * HealRatio;
	if (HealAmount > 0.0f)
	{
		TSubclassOf<UGameplayEffect> ResolvedHealEffectClass = HealEffectClass;
		if (!ResolvedHealEffectClass)
		{
			ResolvedHealEffectClass = LoadClass<UGameplayEffect>(nullptr, TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Healing/GE_Heal_Generic.GE_Heal_Generic_C"));
		}

		if (ResolvedHealEffectClass)
		{
			FGameplayEffectContextHandle HealContextHandle = SourceASC->MakeEffectContext();
			HealContextHandle.AddInstigator(Avatar, Avatar);

			FGameplayEffectSpecHandle HealSpecHandle = SourceASC->MakeOutgoingSpec(ResolvedHealEffectClass, GetAbilityLevel(), HealContextHandle);
			if (HealSpecHandle.IsValid())
			{
				HealSpecHandle.Data->SetSetByCallerMagnitude(FName(TEXT("GPTags.Healing.Data.Base")), HealAmount);
				SourceASC->ApplyGameplayEffectSpecToSelf(*HealSpecHandle.Data.Get());
			}
		}
	}

	if (IsSkillDebugDrawEnabled() && (bDrawDrainDebug || bDrawDebugs))
	{
		if (UWorld* World = GetWorld())
		{
			const FVector Start = Avatar->GetActorLocation() + FVector::UpVector * 70.0f;
			const FVector End = ActiveDrainTarget->GetActorLocation() + FVector::UpVector * 70.0f;
			DrawDebugLine(World, Start, End, FColor::Purple, false, DrainTickInterval, 0, 4.0f);
		}
	}
}

void UGP_Skill_LifeDrainTarget::FinishDrain(bool bWasCancelled)
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}
