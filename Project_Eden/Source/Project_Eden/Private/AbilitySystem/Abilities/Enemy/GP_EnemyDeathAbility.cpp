#include "AbilitySystem/Abilities/Enemy/GP_EnemyDeathAbility.h"

#include "AbilitySystemComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GameplayTags/GP_Tags.h"

UGP_EnemyDeathAbility::UGP_EnemyDeathAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// Death participates in the same tag-based GAS activation contract as enemy attacks and boss patterns.
	FGameplayTagContainer AbilityAssetTags;
	AbilityAssetTags.AddTag(GPTags::Ability::Enemy::Death);
	SetAssetTags(AbilityAssetTags);
}

void UGP_EnemyDeathAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(GetAvatarActorFromActorInfo());
	if (!HasAuthority(&ActivationInfo) || !IsValid(EnemyCharacter))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (UAbilitySystemComponent* ASC = EnemyCharacter->GetAbilitySystemComponent())
	{
		FGameplayTagContainer DeathTags;
		DeathTags.AddTag(GPTags::Ability::Enemy::Death);
		// Cancel combat abilities while excluding this death ability from its own cancellation request.
		ASC->CancelAbilities(nullptr, &DeathTags, this);
	}

	EnemyCharacter->EnterDeathStateFromAbility();
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
