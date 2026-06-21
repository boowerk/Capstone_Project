#pragma once

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace GPBossCombatUtils
{
	/** Shared SetByCaller application keeps prototype boss actors on the project's normal GAS damage path. */
	inline void ApplyDamage(
		AActor* Instigator,
		const TArray<AActor*>& Targets,
		TSubclassOf<UGameplayEffect> EffectClass,
		float AttackPowerCoefficient,
		float BaseDamage = 0.0f,
		bool bMagical = false,
		FGameplayTag ElementTag = FGameplayTag())
	{
		if (!IsValid(Instigator) || !*EffectClass)
		{
			return;
		}

		UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
		if (!IsValid(SourceASC))
		{
			return;
		}

		for (AActor* Target : Targets)
		{
			if (!UGP_BlueprintLibrary::CanApplyCombatEffect(Instigator, Target))
			{
				continue;
		}
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
			if (!IsValid(TargetASC))
			{
				continue;
			}

			FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
			Context.AddInstigator(Instigator, Instigator);
			FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(EffectClass, 1.0f, Context);
			if (!Spec.IsValid())
			{
				continue;
			}

			Spec.Data->AddDynamicAssetTag(bMagical ? GPTags::Damage::Type::Magical : GPTags::Damage::Type::Physical);
			if (ElementTag.IsValid())
			{
				Spec.Data->AddDynamicAssetTag(ElementTag);
			}
			Spec.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, BaseDamage);
			Spec.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, BaseDamage);
			Spec.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, FMath::Max(0.0f, AttackPowerCoefficient));
			Spec.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, FMath::Max(0.0f, AttackPowerCoefficient));
			Spec.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, 1.0f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);

			FGameplayEventData Payload;
			Payload.Instigator = Instigator;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, GPTags::Event::Player::HitReact, Payload);
		}
	}
}
