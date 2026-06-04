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
	UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);
	const TSubclassOf<AActor> SpawnActorClass = GetSkillSpawnActorClass(SkillData, ProjectileClass);

	if (!Avatar || !ASC || !SpawnActorClass || !CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (HasAuthority(&ActivationInfo))
	{
		const FGameplayTag TechElementTag = GetCurrentTechElementTag(ActorInfo);
		const float RadiusMultiplier = GetSkillAugmentRadiusMultiplier(SkillData, ActorInfo);

		FRotator SpawnRotation = Avatar->GetActorRotation();
		if (AController* Controller = Avatar->GetController())
		{
			SpawnRotation = Controller->GetControlRotation();
			SpawnRotation.Pitch = FMath::ClampAngle(SpawnRotation.Pitch, MinPitch, MaxPitch);
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Avatar;
		SpawnParams.Instigator = Avatar;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		const int32 SafeProjectileCount = FMath::Max(1, 1 + GetSkillAugmentProjectileCountBonus(SkillData, ActorInfo));
		constexpr float BonusSpreadAngle = 10.0f;
		const float StepAngle = SafeProjectileCount > 1
			? BonusSpreadAngle / static_cast<float>(SafeProjectileCount - 1)
			: 0.0f;
		const float StartYawOffset = -BonusSpreadAngle * 0.5f;

		for (int32 Index = 0; Index < SafeProjectileCount; ++Index)
		{
			FRotator CurrentSpawnRotation = SpawnRotation;
			CurrentSpawnRotation.Yaw += StartYawOffset + StepAngle * Index;
			const FVector CurrentAimDirection = CurrentSpawnRotation.Vector();
			const FVector CurrentSpawnLocation =
				Avatar->GetActorLocation()
				+ CurrentAimDirection * SpawnForwardOffset
				+ FVector::UpVector * SpawnUpOffset;

			AActor* SpawnedActor = Avatar->GetWorld()->SpawnActor<AActor>(
				SpawnActorClass,
				CurrentSpawnLocation,
				CurrentSpawnRotation,
				SpawnParams
			);
			AGP_AreaProjectile* Projectile = Cast<AGP_AreaProjectile>(SpawnedActor);

			if (Projectile)
			{
				Projectile->SetSkillData(SkillData);
				Projectile->SetImpactVisualActorClass(GetSkillVisualActorClass(SkillData, nullptr, TechElementTag));
				Projectile->SetProjectileVisualSystem(GetProjectileVisualSystem(SkillData, TechElementTag));
				Projectile->ApplyExplosionRadiusMultiplier(RadiusMultiplier);
			}
			else if (SpawnedActor)
			{
				UE_LOG(LogTemp, Warning, TEXT("%s expected AGP_AreaProjectile but spawned %s."), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
			}
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
