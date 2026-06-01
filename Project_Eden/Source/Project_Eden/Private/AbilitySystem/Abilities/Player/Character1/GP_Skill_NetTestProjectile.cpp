// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_NetTestProjectile.h"

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

	if (HasAuthority(&CurrentActivationInfo))
	{
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
		AGP_Projectile* Projectile = Avatar->GetWorld()->SpawnActor<AGP_Projectile>(
			ProjectileClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);

		if (Projectile)
		{
			Projectile->SetSkillData(GetSkillDataFromSpec(CurrentSpecHandle, CurrentActorInfo));
		}
	}
}
