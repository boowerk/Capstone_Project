#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_LightningStrike.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

UGP_Skill_LightningStrike::UGP_Skill_LightningStrike()
{
	SelectionMode = EGP_SkillSelectionMode::GroundPosition;
	bAllowSecondaryConfirm = false;
	bRequireBlockingHit = true;
	HitEventTag = GPTags::Event::Enemy::HitReact;
}

void UGP_Skill_LightningStrike::ExecuteConfirmedSkill_Implementation(
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
	const float RadiusMultiplier = GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo);
	const float FinalRadius = StrikeRadius * RadiusMultiplier;
	const TArray<FGP_NiagaraParameterOverride> NiagaraParameterOverrides =
		GetSkillAugmentNiagaraParameterOverrides(SkillData);

	SpawnVisualActor(
		AvatarActor,
		GetSkillVisualActorClass(
			SkillData,
			StrikeVisualActorClass,
			FGameplayTag(),
			GPTags::Ability::Skill::Visual::Impact),
		TargetData.TargetLocation,
		FRotator::ZeroRotator,
		1.0f,
		NiagaraParameterOverrides);

	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
		AvatarActor,
		TargetData.TargetLocation,
		FinalRadius,
		AvatarActor,
		bDrawDebugs);

	UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
		AvatarActor,
		HitActors,
		DamageEffectClass,
		HitEventTag,
		GetAbilityLevel(),
		SkillData);
}
