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

		AActor* SpawnedActor = Avatar->GetWorld()->SpawnActor<AActor>(
			SpawnActorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParams
		);
		AGP_AreaProjectile* Projectile = Cast<AGP_AreaProjectile>(SpawnedActor);

		if (Projectile)
		{
			Projectile->SetSkillData(SkillData);
			Projectile->SetImpactVisualActorClass(GetSkillVisualActorClass(SkillData, nullptr, TechElementTag));
			Projectile->SetProjectileVisualSystem(GetProjectileVisualSystem(SkillData, TechElementTag));
		}
		else if (SpawnedActor)
		{
			UE_LOG(LogTemp, Warning, TEXT("%s expected AGP_AreaProjectile but spawned %s."), *GetNameSafe(this), *GetNameSafe(SpawnedActor));
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
