#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"

#include "GP_BaseCharacter.generated.h"

class UGameplayAbility;

UCLASS(abstract)
class PROJECT_EDEN_API AGP_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_BaseCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

protected:
	void GiveStartupAbilities();
private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
};
