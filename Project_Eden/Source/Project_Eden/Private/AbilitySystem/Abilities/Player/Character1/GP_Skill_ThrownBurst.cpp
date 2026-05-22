#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_ThrownBurst.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_AreaProjectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

void UGP_Skill_ThrownBurst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo,
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

	if (!Avatar || !ASC || !ProjectileClass || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		FRotator SpawnRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			SpawnRotation = Controller->GetControlRotation();
			SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, MinPitch, MaxPitch);
		}

		const FVector AimDirection = SpawnRotation.Vector();
		const FVector SpawnLocation =
			Avatar->GetActorLocation()
			+ AimDirection * SpawnForwardOffset
			+ FVector::UpVector * SpawnUpOffset;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AGP_AreaProjectile* Projectile = Avatar->GetWorld()->SpawnActor<AGP_AreaProjectile>(
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
