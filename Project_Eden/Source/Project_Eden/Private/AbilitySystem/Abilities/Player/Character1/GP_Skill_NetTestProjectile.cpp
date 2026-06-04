// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_NetTestProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_Projectile.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"

UGP_Skill_NetTestProjectile::UGP_Skill_NetTestProjectile()
{
	SelectionMode = EGP_SkillSelectionMode::Projectile;
}

void UGP_Skill_NetTestProjectile::ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData)
{
	ACharacter* Avatar = Cast<ACharacter>(GetAvatarActorFromActorInfo());
	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const TSubclassOf<AActor> SpawnActorClass = GetSkillSpawnActorClass(SkillData, ProjectileClass);

	if (!Avatar || !ASC || !SpawnActorClass)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&CurrentActivationInfo) && SpawnActorClass)
	{
		const FGameplayTag TechElementTag = GetCurrentTechElementTag(CurrentActorInfo);
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const int32 SafeProjectileCount = FMath::Max(1, 1 + GetSkillAugmentProjectileCountBonus(SkillData, CurrentActorInfo));
		constexpr float BonusSpreadAngle = 10.0f;
		const float StepAngle = SafeProjectileCount > 1
			? BonusSpreadAngle / static_cast<float>(SafeProjectileCount - 1)
			: 0.0f;
		const float StartYawOffset = -BonusSpreadAngle * 0.5f;

		for (int32 Index = 0; Index < SafeProjectileCount; ++Index)
		{
			FRotator SpawnRotation = TargetData.AimDirection.Rotation();
			SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, -5.f, 10.f);
			SpawnRotation.Yaw += StartYawOffset + StepAngle * Index;

			const FVector AimDirection = SpawnRotation.Vector();
			const FVector SpawnLocation =
				Avatar->GetActorLocation()
				+ AimDirection * SpawnForwardOffset
				+ FVector::UpVector * SpawnUpOffset;

			AActor* SpawnedActor = Avatar->GetWorld()->SpawnActor<AActor>(
				SpawnActorClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams
			);
			AGP_Projectile* Projectile = Cast<AGP_Projectile>(SpawnedActor);

			if (Projectile)
			{
				Projectile->SetSkillData(SkillData);
				Projectile->SetProjectileVisualSystem(GetProjectileVisualSystem(SkillData, TechElementTag));
			}
			else if (SpawnedActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s expected AGP_Projectile but spawned %s."), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
			}
		}
	}
}
