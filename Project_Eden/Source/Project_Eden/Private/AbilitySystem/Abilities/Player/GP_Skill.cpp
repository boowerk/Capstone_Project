#include "AbilitySystem/Abilities/Player/GP_Skill.h"

UGP_Skill::UGP_Skill()
{
	// 어빌리티 인스턴싱 정책 일반적으로 액션 어빌리티는 InstancedPerActor를 사용하도록하자
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGP_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (bDrawDebugs)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skill Ability Activated!"));
	}
	
	// TODO: 느들이 할건데 Play Animation Montage (PlayMontageAndWait 등 Task 사용)
	// TODO: Apply Cooldown 랑 Cost Gameplay Effects 추가
}

void UGP_Skill::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}