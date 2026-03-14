#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"

#include "GP_AbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()


public:
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRep_ActivateAbilities() override;
	
private:
	void HandleAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec);
	
};
