#include "Player/GP_PlayerState.h"
#include "AbilitySystem/GP_AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "AbilitySystemComponent.h"
#include "Engine/Engine.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "GameplayTags/GP_Tags.h"


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

	if (SelectedSkillAugments.Contains(AugmentData))
	{
		return;
	}

	SelectedSkillAugments.Add(AugmentData);

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
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
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
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
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
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
		{
			continue;
		}

		RangeMultiplier *= FMath::Max(Augment->Modifiers.RangeMultiplier, 0.0f);
	}

	return RangeMultiplier;
}

float AGP_PlayerState::GetSkillAugmentCooldownMultiplier(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return 1.0f;
	}

	float CooldownMultiplier = 1.0f;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
		{
			continue;
		}

		CooldownMultiplier *= FMath::Max(Augment->Modifiers.CooldownMultiplier, 0.0f);
	}

	return CooldownMultiplier;
}

int32 AGP_PlayerState::GetSkillAugmentProjectileCountBonus(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return 0;
	}

	int32 ProjectileCountBonus = 0;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
		{
			continue;
		}

		ProjectileCountBonus += FMath::Max(Augment->Modifiers.ProjectileCountBonus, 0);
	}

	return ProjectileCountBonus;
}

bool AGP_PlayerState::HasSkillAugmentInfiniteProjectilePierce(FGameplayTag SkillIdTag) const
{
	if (!SkillIdTag.IsValid())
	{
		return false;
	}

	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
		{
			continue;
		}

		if (Augment->Modifiers.bProjectileInfinitePierce)
		{
			return true;
		}
	}

	return false;
}

TSubclassOf<AActor> AGP_PlayerState::GetSkillAugmentImpactVisualActorOverride(FGameplayTag SkillIdTag) const
{
	for (int32 Index = SelectedSkillAugments.Num() - 1; Index >= 0; --Index)
	{
		const UGP_SkillAugmentData* Augment = SelectedSkillAugments[Index];
		if (DoesAugmentApplyToSkill(Augment, SkillIdTag) && Augment->ImpactVisualActorOverride)
		{
			return Augment->ImpactVisualActorOverride;
		}
	}

	return nullptr;
}

UNiagaraSystem* AGP_PlayerState::GetSkillAugmentActiveVFXOverride(FGameplayTag SkillIdTag) const
{
	for (int32 Index = SelectedSkillAugments.Num() - 1; Index >= 0; --Index)
	{
		const UGP_SkillAugmentData* Augment = SelectedSkillAugments[Index];
		if (DoesAugmentApplyToSkill(Augment, SkillIdTag) && Augment->ActiveVFXOverride)
		{
			return Augment->ActiveVFXOverride;
		}
	}

	return nullptr;
}

TArray<FGP_NiagaraParameterOverride> AGP_PlayerState::GetSkillAugmentNiagaraParameterOverrides(FGameplayTag SkillIdTag) const
{
	TArray<FGP_NiagaraParameterOverride> Overrides;
	if (!SkillIdTag.IsValid())
	{
		return Overrides;
	}

	TMap<FName, int32> OverrideIndices;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (!DoesAugmentApplyToSkill(Augment, SkillIdTag))
		{
			continue;
		}

		for (const FGP_NiagaraParameterOverride& Override : Augment->NiagaraParameterOverrides)
		{
			if (Override.ParameterName.IsNone())
			{
				continue;
			}

			if (const int32* ExistingIndex = OverrideIndices.Find(Override.ParameterName))
			{
				Overrides[*ExistingIndex] = Override;
				continue;
			}

			OverrideIndices.Add(Override.ParameterName, Overrides.Add(Override));
		}
	}

	return Overrides;
}

FGP_SkillAugmentPeriodicAreaDamage AGP_PlayerState::GetSkillAugmentPeriodicAreaDamage(FGameplayTag SkillIdTag) const
{
	FGP_SkillAugmentPeriodicAreaDamage Settings;
	for (const UGP_SkillAugmentData* Augment : SelectedSkillAugments)
	{
		if (DoesAugmentApplyToSkill(Augment, SkillIdTag) && Augment->PeriodicAreaDamage.bEnabled)
		{
			Settings = Augment->PeriodicAreaDamage;
		}
	}

	return Settings;
}

void AGP_PlayerState::AddXP(float Amount)
{
	if (Amount <= 0.0f)
	{
		return;
	}

	if (!HasAuthority())
	{
		ServerAddXP(Amount);
		return;
	}

	const int32 PreviousLevel = CurrentLevel;
	CurrentXP += Amount;
	bool bLeveledUp = false;

	while (XPToNextLevel > 0.0f && CurrentXP >= XPToNextLevel)
	{
		CurrentXP -= XPToNextLevel;
		++CurrentLevel;
		XPToNextLevel = FMath::Max(1.0f, XPToNextLevel * LevelXPScale);
		bLeveledUp = true;
	}

	if (bLeveledUp)
	{
		OnLevelUp(CurrentLevel);
	}

	if (bDebugXPChanges)
	{
		MulticastShowXPDebug(Amount, PreviousLevel, CurrentLevel, CurrentXP, XPToNextLevel);
	}

	ForceNetUpdate();
}

void AGP_PlayerState::ServerAddXP_Implementation(float Amount)
{
	AddXP(Amount);
}

void AGP_PlayerState::SetEquippedSkillData(FGameplayTag SlotTag, UGP_SkillData* SkillData)
{
	if (!HasAuthority())
	{
		return;
	}

	if (SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot01))
	{
		Slot01SkillData = SkillData;
		OnEquippedSkillChanged.Broadcast(SlotTag, SkillData);
	}
	else if (SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot02))
	{
		Slot02SkillData = SkillData;
		OnEquippedSkillChanged.Broadcast(SlotTag, SkillData);
	}
	else
	{
		return;
	}

	ForceNetUpdate();
}

UGP_SkillData* AGP_PlayerState::GetEquippedSkillData(FGameplayTag SlotTag) const
{
	if (SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot01))
	{
		return Slot01SkillData;
	}

	if (SlotTag.MatchesTagExact(GPTags::Ability::Skill::Slot02))
	{
		return Slot02SkillData;
	}

	return nullptr;
}

void AGP_PlayerState::MulticastShowXPDebug_Implementation(float AddedXP, int32 PreviousLevel, int32 NewLevel, float NewXP, float NewXPToNextLevel)
{
	if (!bDebugXPChanges)
	{
		return;
	}

	const FString DebugMessage = FString::Printf(
		TEXT("[XP Debug] +%.1f XP | Lv %d -> %d | %.1f / %.1f"),
		AddedXP,
		PreviousLevel,
		NewLevel,
		NewXP,
		NewXPToNextLevel);

	UE_LOG(LogTemp, Log, TEXT("%s"), *DebugMessage);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, DebugXPMessageDuration, FColor::Green, DebugMessage);
	}
}

bool AGP_PlayerState::DoesAugmentApplyToSkill(const UGP_SkillAugmentData* AugmentData, FGameplayTag SkillIdTag) const
{
	if (!IsValid(AugmentData) || !SkillIdTag.IsValid())
	{
		return false;
	}

	const bool bMatchesSkill = AugmentData->TargetSkillTags.IsEmpty() || AugmentData->TargetSkillTags.HasTagExact(SkillIdTag);
	if (!bMatchesSkill)
	{
		return false;
	}

	return true;
}

void AGP_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_PlayerState, CurrentTechElementTag);
	DOREPLIFETIME(AGP_PlayerState, SelectedSkillAugments);
	DOREPLIFETIME(AGP_PlayerState, CurrentXP);
	DOREPLIFETIME(AGP_PlayerState, CurrentLevel);
	DOREPLIFETIME(AGP_PlayerState, XPToNextLevel);
	DOREPLIFETIME(AGP_PlayerState, Slot01SkillData);
	DOREPLIFETIME(AGP_PlayerState, Slot02SkillData);
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

void AGP_PlayerState::OnRep_CurrentXP()
{
}

void AGP_PlayerState::OnRep_CurrentLevel(int32 PreviousLevel)
{
	if (CurrentLevel > PreviousLevel)
	{
		OnLevelUp(CurrentLevel);
	}
}

void AGP_PlayerState::OnRep_XPToNextLevel()
{
}

void AGP_PlayerState::OnRep_Slot01SkillData()
{
	OnEquippedSkillChanged.Broadcast(GPTags::Ability::Skill::Slot01, Slot01SkillData);
}

void AGP_PlayerState::OnRep_Slot02SkillData()
{
	OnEquippedSkillChanged.Broadcast(GPTags::Ability::Skill::Slot02, Slot02SkillData);
}
