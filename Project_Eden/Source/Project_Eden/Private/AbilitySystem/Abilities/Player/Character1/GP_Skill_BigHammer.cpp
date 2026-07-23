#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_BigHammer.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_BigHammerDropActor.h"
#include "GameplayTags/GP_Tags.h"

UGP_Skill_BigHammer::UGP_Skill_BigHammer()
{
	SelectionMode = EGP_SkillSelectionMode::GroundPosition;
	bAllowSecondaryConfirm = false;
	bRequireBlockingHit = true;
	bUseProductionTargetPreview = true;
	PreviewVisualStyle = EGP_SkillTargetPreviewStyle::HeavyImpact;
	HitEventTag = GPTags::Event::Enemy::HitReact;
}

void UGP_Skill_BigHammer::ExecuteConfirmedSkill_Implementation(
	const FGP_SkillTargetData& TargetData)
{
	if (!HasAuthority(&CurrentActivationInfo))
	{
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!IsValid(AvatarActor))
	{
		return;
	}

	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const TSubclassOf<AActor> SpawnActorClass =
		GetSkillSpawnActorClass(SkillData, HammerDropActorClass);
	if (!SpawnActorClass)
	{
		return;
	}

	const FVector DamageImpactLocation = TargetData.TargetLocation;
	const FVector HammerDropEndLocation =
		TargetData.TargetLocation + FVector::UpVector * ImpactZOffset;
	const FVector SpawnLocation =
		HammerDropEndLocation + FVector::UpVector * FMath::Max(0.0f, DropHeight);

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor;
	SpawnParams.Instigator = Cast<APawn>(AvatarActor);
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_BigHammerDropActor* HammerActor =
		AvatarActor->GetWorld()->SpawnActor<AGP_BigHammerDropActor>(
			SpawnActorClass,
			SpawnLocation,
			HammerRotation,
			SpawnParams);
	if (!HammerActor)
	{
		return;
	}

	HammerActor->InitializeDrop(
		AvatarActor,
		SkillData,
		DamageEffectClass,
		HitEventTag,
		GetAbilityLevel(),
		HammerDropEndLocation,
		DamageImpactLocation,
		GetFinalImpactRadius(SkillData),
		DropDuration,
		GetSkillVisualActorClass(
			SkillData,
			nullptr,
			FGameplayTag(),
			GPTags::Ability::Skill::Visual::Impact),
		GetSkillAugmentNiagaraParameterOverrides(SkillData),
		bDrawDebugs);
}

float UGP_Skill_BigHammer::GetPreviewActorRadius() const
{
	return GetFinalImpactRadius(
		GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo));
}

float UGP_Skill_BigHammer::GetFinalImpactRadius(const UGP_SkillData* SkillData) const
{
	return ImpactRadius * GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo);
}
