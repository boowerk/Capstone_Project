#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"

#include "GP_BaseCharacter.generated.h"

class UGameplayAbility;
class AGP_DamageNumberActor;
enum class EWeaponElement : uint8;

UCLASS(abstract)
class PROJECT_EDEN_API AGP_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_BaseCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	void ShowDamageNumber(int32 DamageAmount, EWeaponElement Element);

protected:
	void GiveStartupAbilities();

	UPROPERTY(EditDefaultsOnly, Category = "UI|Damage")
	TSubclassOf<AGP_DamageNumberActor> DamageNumberActorClass;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShowDamageNumber(int32 DamageAmount, EWeaponElement Element);

	void SpawnDamageNumberActor(int32 DamageAmount, EWeaponElement Element);
private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
};

