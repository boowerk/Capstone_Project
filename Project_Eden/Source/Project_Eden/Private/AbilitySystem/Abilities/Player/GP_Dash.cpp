#include "AbilitySystem/Abilities/Player/GP_Dash.h"

UGP_Dash::UGP_Dash()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGP_Dash::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (bDrawDebugs)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dash Ability Activated!"));
	}
	
	// TODO: 내가 아마 구현할것 플레이어 입력 방향에 따른 회피 방향 계산 - 슝
	// TODO: 회피 몽타주 재생 및 무적 태그(Gameplay Effect) 적용
}