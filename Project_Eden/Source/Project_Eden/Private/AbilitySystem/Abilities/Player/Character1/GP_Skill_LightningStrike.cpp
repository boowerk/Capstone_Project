#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_LightningStrike.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_PeriodicAreaDamageActor.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

namespace
{
void UpsertFloatNiagaraOverride(
	TArray<FGP_NiagaraParameterOverride>& Overrides,
	FName ParameterName,
	float Value)
{
	if (ParameterName.IsNone())
	{
		return;
	}

	for (FGP_NiagaraParameterOverride& Override : Overrides)
	{
		if (Override.ParameterName == ParameterName)
		{
			Override = FGP_NiagaraParameterOverride::MakeFloat(ParameterName, Value);
			return;
		}
	}

	Overrides.Add(FGP_NiagaraParameterOverride::MakeFloat(ParameterName, Value));
}
}

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
	const FGP_SkillAugmentPeriodicAreaDamage PeriodicAreaDamage =
		GetSkillAugmentPeriodicAreaDamage(SkillData);
	const float FinalRadius = GetFinalStrikeRadius(SkillData);
	TArray<FGP_NiagaraParameterOverride> NiagaraParameterOverrides =
		GetSkillAugmentNiagaraParameterOverrides(SkillData);

	if (PeriodicAreaDamage.bEnabled && SkillData)
	{
		const FGP_PeriodicAreaNiagaraBindings& Bindings =
			SkillData->PeriodicAreaNiagaraBindings;
		UpsertFloatNiagaraOverride(
			NiagaraParameterOverrides,
			Bindings.RadiusParameterName,
			PeriodicAreaDamage.Radius);
		UpsertFloatNiagaraOverride(
			NiagaraParameterOverrides,
			Bindings.DurationParameterName,
			PeriodicAreaDamage.Duration);
		UpsertFloatNiagaraOverride(
			NiagaraParameterOverrides,
			Bindings.SpawnRateParameterName,
			1.0f / FMath::Max(PeriodicAreaDamage.Interval, 0.01f));
	}

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

	if (PeriodicAreaDamage.bEnabled &&
		PeriodicAreaDamage.Duration > 0.0f &&
		PeriodicAreaDamage.DamagePerTickMultiplier > 0.0f)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = AvatarActor;
		SpawnParams.Instigator = Cast<APawn>(AvatarActor);
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.bDeferConstruction = true;

		if (AGP_PeriodicAreaDamageActor* PeriodicDamageActor =
			AvatarActor->GetWorld()->SpawnActor<AGP_PeriodicAreaDamageActor>(
				AGP_PeriodicAreaDamageActor::StaticClass(),
				TargetData.TargetLocation,
				FRotator::ZeroRotator,
				SpawnParams))
		{
			PeriodicDamageActor->InitializePeriodicDamage(
				AvatarActor,
				SkillData,
				DamageEffectClass,
				HitEventTag,
				GetAbilityLevel(),
				FinalRadius,
				PeriodicAreaDamage.Duration,
				PeriodicAreaDamage.Interval,
				PeriodicAreaDamage.DamagePerTickMultiplier,
				bDrawDebugs);
			PeriodicDamageActor->FinishSpawning(
				FTransform(FRotator::ZeroRotator, TargetData.TargetLocation));
		}
	}
}

float UGP_Skill_LightningStrike::GetPreviewActorRadius() const
{
	const UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	return GetFinalStrikeRadius(SkillData);
}

float UGP_Skill_LightningStrike::GetFinalStrikeRadius(const UGP_SkillData* SkillData) const
{
	const float BaseRadius = StrikeRadius * GetSkillAugmentRadiusMultiplier(SkillData, CurrentActorInfo);
	const FGP_SkillAugmentPeriodicAreaDamage PeriodicAreaDamage =
		GetSkillAugmentPeriodicAreaDamage(SkillData);
	return PeriodicAreaDamage.bEnabled
		? FMath::Max(BaseRadius, PeriodicAreaDamage.Radius)
		: BaseRadius;
}
