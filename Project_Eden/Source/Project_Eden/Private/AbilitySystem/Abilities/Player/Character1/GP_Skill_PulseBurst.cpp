#include "AbilitySystem/Abilities/Player/Character1/GP_Skill_PulseBurst.h"

#include "AbilitySystemComponent.h"
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
		UGP_SkillData* SkillData = GetSkillDataFromSpec(Handle, ActorInfo);

		const FVector BurstLocation = Avatar->GetActorLocation();
		SpawnVisualActor(Avatar, BurstVisualActorClass, BurstLocation);

		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			Avatar,
			BurstLocation,
			BurstRadius,
			Avatar,
			bDrawDebugs
		);

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(Avatar, HitActors, DamageEffectClass, HitEventTag, GetAbilityLevel(), SkillData);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

UGP_Skill_PulseBurst::UGP_Skill_PulseBurst()
{
	HitEventTag = GPTags::Event::Enemy::HitReact;
}
