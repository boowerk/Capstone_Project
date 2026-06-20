#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_EnemyDeathAbility.generated.h"

struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_EnemyDeathAbility : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_EnemyDeathAbility();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;
};
