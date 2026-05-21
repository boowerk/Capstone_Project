#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_SplitShot.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Actors/GP_Projectile.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"

void UGP_Skill_SplitShot::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (!Avatar || !ASC || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo) && ProjectileClass)
	{
		UGP_SkillData* SkillData = nullptr;
		if (const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle))
		{
			SkillData = Cast<UGP_SkillData>(Spec->SourceObject.Get());
		}

		FRotator AimRotation = Avatar->GetActorRotation();

		if (AController* Controller = Avatar->GetController())
		{
			AimRotation = Controller->GetControlRotation();
			AimRotation.Pitch = FMath::ClampAngle(AimRotation.Pitch, -5.f, 10.f);
		}

		const int32 SafeProjectileCount = FMath::Max(1, ProjectileCount);
		const float StepAngle = SafeProjectileCount > 1
			? TotalSpreadAngle / static_cast<float>(SafeProjectileCount - 1)
			: 0.f;
		const float StartYawOffset = -TotalSpreadAngle * 0.5f;

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		for (int32 Index = 0; Index < SafeProjectileCount; ++Index)
		{
			FRotator SpawnRotation = AimRotation;
			SpawnRotation.Yaw += StartYawOffset + StepAngle * Index;

			const FVector AimDirection = SpawnRotation.Vector();
			const FVector SpawnLocation =
				Avatar->GetActorLocation()
				+ AimDirection * SpawnForwardOffset
				+ FVector::UpVector * SpawnUpOffset;

			AGP_Projectile* Projectile = Avatar->GetWorld()->SpawnActor<AGP_Projectile>(
				ProjectileClass,
				SpawnLocation,
				SpawnRotation,
				SpawnParams
			);

			if (Projectile)
			{
				Projectile->SetSkillData(SkillData);
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
