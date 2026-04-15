#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"

#include "GP_BaseCharacter.generated.h"

class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class AGP_DamageNumberActor;
enum class EWeaponElement : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(abstract)
class PROJECT_EDEN_API AGP_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_BaseCharacter();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }
	
	void ShowDamageNumber(int32 DamageAmount, EWeaponElement Element);
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

protected:
	void GiveStartupAbilities();
	void InitializeAttributes() const;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Damage")
	TSubclassOf<AGP_DamageNumberActor> DamageNumberActorClass;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShowDamageNumber(int32 DamageAmount, EWeaponElement Element);

	void SpawnDamageNumberActor(int32 DamageAmount, EWeaponElement Element);
	
	// ASC가 초기화되었을 때 AttributeSet의 델리게이트를 구독할 함수
	UFUNCTION()
	void BindAttributeDelegates(UAbilitySystemComponent* ASC, UAttributeSet* AS);

	// 데미지 델리게이트 수신용 함수
	UFUNCTION()
	void HandleDamageTaken(AActor* InstigatorActor, AActor* TargetActor, float DamageAmount, FGameplayTag ElementTag);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
};

