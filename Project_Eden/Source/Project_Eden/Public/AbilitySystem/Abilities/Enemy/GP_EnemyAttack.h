#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GP_EnemyAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class PROJECT_EDEN_API UGP_EnemyAttack : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_EnemyAttack();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 공격 몽타주가 있으면 재생하고, 없으면 즉시 판정을 수행한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities|Attack")
	TObjectPtr<UAnimMontage> AttackMontage;

	// 정밀한 타격 타이밍이 필요할 때 몽타주 노티파이에서 이 태그를 보낸다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities|Attack")
	FGameplayTag AttackEventTag;

	// true면 Gameplay Event 시점에 판정을 내고, false면 어빌리티 시작 즉시 판정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities|Attack")
	bool bUseGameplayEventForHitTiming = false;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities|Attack")
	float HitBoxRadius = 120.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities|Attack")
	float HitBoxForwardOffset = 140.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities|Attack")
	float HitBoxElevationOffset = 40.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);

	void PerformAttackHit();

	bool bHasAppliedAttackHit = false;
};
