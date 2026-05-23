#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_LineShock.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/Controller.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

void UGP_Skill_LineShock::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	if (HasAuthority(&ActivationInfo))
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
		}

		AimRotation.Pitch = 0.f;
		AimRotation.Roll = 0.f;

		const FVector Forward = AimRotation.Vector();
		const FVector BoxCenter =
			Avatar->GetActorLocation()
			+ Forward * (Range * 0.5f)
			+ FVector::UpVector * HeightOffset;
		const FVector BoxExtent(Range * 0.5f, Width * 0.5f, Height * 0.5f);

		if (ShockVisualActorClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Avatar;
			SpawnParams.Instigator = Avatar;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			Avatar->GetWorld()->SpawnActor<AActor>(
				ShockVisualActorClass,
				BoxCenter,
				AimRotation,
				SpawnParams
			);
		}

		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::BoxOverlapActorsAtLocation(
			Avatar,
			BoxCenter,
			BoxExtent,
			AimRotation,
			Avatar,
			bDrawDebugs
		);

		if (DamageEffectClass)
		{
			UGP_BlueprintLibrary::ApplyGameplayEffectToActors(Avatar, HitActors, DamageEffectClass, GetAbilityLevel(), SkillData);
		}

		if (HitEventTag.IsValid())
		{
			UGP_BlueprintLibrary::SendGameplayEventToActors(Avatar, HitActors, HitEventTag);
		}
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGP_Skill_LineShock::UGP_Skill_LineShock()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
