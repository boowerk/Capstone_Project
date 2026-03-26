#include "AbilitySystem/Abilities/GP_GameplayAbility.h"

void UGP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (bDrawDebugs && IsValid(GEngine))
	{
		const FString DebugMessage = FString::Printf(TEXT("%s Activated"), *GetName());
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Cyan, DebugMessage);
	}
}
