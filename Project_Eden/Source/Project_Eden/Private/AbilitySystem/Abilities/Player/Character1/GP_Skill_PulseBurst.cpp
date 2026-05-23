#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_PulseBurst.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GP_Tags.h"
#include "Utils/GP_BlueprintLibrary.h"

void UGP_Skill_PulseBurst::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

		const FVector BurstLocation = Avatar->GetActorLocation();

		if (BurstVisualActorClass)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Avatar;
			SpawnParams.Instigator = Avatar;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			Avatar->GetWorld()->SpawnActor<AActor>(
				BurstVisualActorClass,
				BurstLocation,
				FRotator::ZeroRotator,
				SpawnParams
			);
		}

		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			Avatar,
			BurstLocation,
			BurstRadius,
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

UGP_Skill_PulseBurst::UGP_Skill_PulseBurst()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
