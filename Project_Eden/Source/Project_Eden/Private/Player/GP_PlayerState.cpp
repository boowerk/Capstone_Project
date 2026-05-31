#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/GP_AttributeSet.h"


AGP_PlayerState::AGP_PlayerState()
{
	SetNetUpdateFrequency(100.f);

	AbilitySystemComponent = CreateDefaultSubobject<UGP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UGP_AttributeSet>("AttributeSet");
}

UAbilitySystemComponent* AGP_PlayerState::GetAbilitySystemComponent() const {
	return AbilitySystemComponent;
}

bool AGP_PlayerState::EquipWeaponFromCollection(UPDA_WeaponItemCollection* WeaponCollection, FName WeaponId)
{
	if (!HasAuthority() || !IsValid(AbilitySystemComponent) || !IsValid(WeaponCollection) || WeaponId.IsNone())
	{
		return false;
	}

	FWeaponItemData WeaponData;
	if (!WeaponCollection->GetWeaponDataById(WeaponId, WeaponData))
	{
		return false;
	}
	

	ForceNetUpdate();
	return true;
}

void AGP_PlayerState::SetCurrentTechElementTag(FGameplayTag NewElementTag)
{
	if (!HasAuthority())
	{
		ServerSetCurrentTechElementTag(NewElementTag);
		return;
	}

	CurrentTechElementTag = NewElementTag;
	ForceNetUpdate();
}

void AGP_PlayerState::ServerSetCurrentTechElementTag_Implementation(FGameplayTag NewElementTag)
{
	SetCurrentTechElementTag(NewElementTag);
}

void AGP_PlayerState::AddSkillAugment(UGP_SkillAugmentData* AugmentData)
{
	if (!IsValid(AugmentData))
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerAddSkillAugment(AugmentData);
		return;
	}

	SelectedSkillAugments.Add(AugmentData);

	if (AugmentData->GrantedElementTag.IsValid())
	{
		CurrentTechElementTag = AugmentData->GrantedElementTag;
	}

	ForceNetUpdate();
}

void AGP_PlayerState::ServerAddSkillAugment_Implementation(UGP_SkillAugmentData* AugmentData)
{
	AddSkillAugment(AugmentData);
}

TArray<UGP_SkillAugmentData*> AGP_PlayerState::GetSelectedSkillAugments() const
{
	TArray<UGP_SkillAugmentData*> Augments;
	Augments.Reserve(SelectedSkillAugments.Num());

	for (UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		Augments.Add(Augment);
	}

	return Augments;
}

float AGP_PlayerState::GetSkillAugmentDamageMultiplier(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	float DamageMultiplier = 1.0f;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!IsValid(Augment) || !Augment->TargetSkillTags.HasTagExact(SkillIdTag))
		{
			continue;
		}

		DamageMultiplier *= FMath::Max(Augment->Modifiers.DamageMultiplier, 0.0f);
	}

	return DamageMultiplier;
}

float AGP_PlayerState::GetSkillAugmentRadiusMultiplier(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	float RadiusMultiplier = 1.0f;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!IsValid(Augment) || !Augment->TargetSkillTags.HasTagExact(SkillIdTag))
		{
			continue;
		}

		RadiusMultiplier *= FMath::Max(Augment->Modifiers.RadiusMultiplier, 0.0f);
	}

	return RadiusMultiplier;
}

float AGP_PlayerState::GetSkillAugmentRangeMultiplier(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	float RangeMultiplier = 1.0f;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!IsValid(Augment) || !Augment->TargetSkillTags.HasTagExact(SkillIdTag))
		{
			continue;
		}

		RangeMultiplier *= FMath::Max(Augment->Modifiers.RangeMultiplier, 0.0f);
	}

	return RangeMultiplier;
}

void AGP_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_PlayerState, CurrentTechElementTag);
	DOREPLIFETIME(AGP_PlayerState, SelectedSkillAugments);
}

void AGP_PlayerState::OnRep_EquippedWeaponData()
{
}

void AGP_PlayerState::OnRep_CurrentTechElementTag()
{
}

void AGP_PlayerState::OnRep_SelectedSkillAugments()
{
}
