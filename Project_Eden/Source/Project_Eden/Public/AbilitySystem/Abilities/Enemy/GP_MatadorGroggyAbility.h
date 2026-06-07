#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_MatadorGroggyAbility.generated.h"

struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_MatadorGroggyAbility : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_MatadorGroggyAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
