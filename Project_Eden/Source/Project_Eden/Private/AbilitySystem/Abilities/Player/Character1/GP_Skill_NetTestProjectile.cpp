// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_NetTestProjectile.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_Projectile.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"

void UGP_Skill_NetTestProjectile::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!ActorInfo)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Avatar = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (!Avatar || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo) && ProjectileClass)
	{
		FRotator SpawnRotation = Avatar->GetActorRotation();

		if (AController* Controller = Avatar->GetController())
		{
			SpawnRotation = Controller->GetControlRotation();
			SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, -5.f, 10.f);
		}

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
			if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
			{
				Projectile->SetSkillData(Cast<UGP_SkillData>(Spec->SourceObject.Get()));
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
