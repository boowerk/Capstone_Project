#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_MatadorBullPatternAbility.generated.h"

struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_MatadorBullPatternAbility : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_MatadorBullPatternAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
