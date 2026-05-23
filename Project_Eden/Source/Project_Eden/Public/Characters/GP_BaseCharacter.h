#pragma once
#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Engine/EngineTypes.h"
#include "UObject/ObjectPtr.h"

#include "GP_BaseCharacter.generated.h"

class UAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class AActor;
class AGP_DamageNumberActor;
class UPDA_CharacterAnimationSet;
enum class EWeaponElement : uint8;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(abstract)
class PROJECT_EDEN_API AGP_BaseCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_BaseCharacter();
	virtual void PostInitializeComponents() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return nullptr; }
	
	void ShowDamageNumber(int32 DamageAmount, EWeaponElement Element);
	void ShowSkillVisualActor(TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation);

	/** 캐릭터의 외형과 애니메이션을 결정하는 데이터 에셋 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;

	/** 데이터 에셋을 바탕으로 메시와 애니메이션 인스턴스를 업데이트합니다. */
	UFUNCTION(BlueprintCallable, Category = "Animation")
	void UpdateAnimationSet();
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

protected:
	void GiveStartupAbilities();
	void InitializeAttributes() const;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Damage")
	TSubclassOf<AGP_DamageNumberActor> DamageNumberActorClass;

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastShowDamageNumber(int32 DamageAmount, EWeaponElement Element);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnSkillVisualActor(TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation);

	void SpawnDamageNumberActor(int32 DamageAmount, EWeaponElement Element);
	void SpawnSkillVisualActor(TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation);
	
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

