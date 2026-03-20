#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Ultimate.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_Ultimate : public UGP_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UGP_Ultimate();

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};