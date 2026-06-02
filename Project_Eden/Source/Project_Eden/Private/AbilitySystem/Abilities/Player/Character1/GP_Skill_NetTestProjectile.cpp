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
	if (!Avatar || !ProjectileClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = CurrentActorInfo ? CurrentActorInfo->AbilitySystemComponent.Get() : nullptr;
	UGP_SkillData* SkillData = GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo);
	const TSubclassOf<AActor> SpawnActorClass = GetSkillSpawnActorClass(SkillData, ProjectileClass);

	if (!ASC || !CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&CurrentActivationInfo) && SpawnActorClass)
	{
		const FGameplayTag TechElementTag = GetCurrentTechElementTag(CurrentActorInfo);
		FRotator SpawnRotation = TargetData.AimDirection.Rotation();
		SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, -5.f, 10.f);

		const FVector AimDirection = SpawnRotation.Vector();
		const FVector SpawnLocation =
			Avatar->GetActorLocation()
			+ AimDirection * SpawnForwardOffset
			+ FVector::UpVector * SpawnUpOffset;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// --- 스폰 ---
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
