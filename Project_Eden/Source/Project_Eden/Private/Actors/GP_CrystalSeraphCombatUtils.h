#pragma once

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace GPCrystalSeraphCombatUtils
{
	inline void ApplySetByCallerDamage(
		AActor* Instigator,
		const TArray<AActor*>& TargetActors,
		TSubclassOf<UGameplayEffect> EffectClass,
		FGameplayTag EventTag,
		float AttackPowerCoefficient,
		float MaxHealthCoefficient,
		float BaseDamage,
		bool bMagicalDamage)
	{
		if (!IsValid(Instigator) || !*EffectClass)
		{
			return;
		}

		UAbilitySystemComponent* InstigatorASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Instigator);
		if (!IsValid(InstigatorASC))
		{
			return;
		}

		for (AActor* TargetActor : TargetActors)
		{
			if (!UGP_BlueprintLibrary::CanApplyCombatEffect(Instigator, TargetActor))
			{
				continue;
			}

			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
			if (!IsValid(TargetASC))
			{
				continue;
			}

			FGameplayEffectContextHandle ContextHandle = InstigatorASC->MakeEffectContext();
			ContextHandle.AddInstigator(Instigator, Instigator);

			FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(EffectClass, 1.0f, ContextHandle);
			if (!SpecHandle.IsValid())
			{
				continue;
			}

			if (bMagicalDamage)
			{
				// Crystal Seraph patterns are treated as magical light/crystal damage in the shared damage execution.
				SpecHandle.Data->AddDynamicAssetTag(GPTags::Damage::Type::Magical);
				SpecHandle.Data->AddDynamicAssetTag(GPTags::Damage::Element::Lux);
			}

			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, BaseDamage);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, BaseDamage);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, FMath::Max(0.0f, AttackPowerCoefficient));
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, FMath::Max(0.0f, AttackPowerCoefficient));
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, FMath::Max(0.0f, MaxHealthCoefficient));
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, 1.0f);

			InstigatorASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
			if (EventTag.IsValid())
			{
				FGameplayEventData Payload;
				Payload.Instigator = Instigator;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, Payload);
			}
		}
	}
}
