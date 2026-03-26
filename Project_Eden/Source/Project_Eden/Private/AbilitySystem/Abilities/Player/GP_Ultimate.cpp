#include "AbilitySystem/Abilities/Player/GP_Ultimate.h"

UGP_Ultimate::UGP_Ultimate()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGP_Ultimate::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (bDrawDebugs)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ultimate Ability Activated!"));
	}
	
	// TODO: 궁극기 컷신 연출, 시간 정지 또는 슬로우 모션(Time Dilation) 처리
}