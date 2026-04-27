#include "AbilitySystem/Abilities/Player/GP_Sprint.h"
#include "GameplayTags/GP_Tags.h"

UGP_Sprint::UGP_Sprint()
{
	// InstancedPerActor
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	// 어빌리티의 식별 태그 GP_PlayerCharacter::ToggleSprinting
	AbilityTags.AddTag(GPTags::State::Movement::Sprinting);
	// 어빌리티 활성기간 캐릭터에게 부여할 태그 GP_PlayerCharacter::OnSprintingTagChanged가 반응
	ActivationOwnedTags.AddTag(GPTags::State::Movement::Sprinting);
}

void UGP_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 스프린트는 몽타주 재생 없이 '상태'만 유지하면 되므로, 
	// EndAbility를 명시적으로 호출하지 않고 대기합니다.
	// (종료는 플레이어 컨트롤러에서 CancelAbilities를 호출할 때 자동으로 이루어짐)
}

void UGP_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 어빌리티가 종료될 때 자동으로 ActivationOwnedTags가 제거되며 속도가 원상복구됩니다.
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
