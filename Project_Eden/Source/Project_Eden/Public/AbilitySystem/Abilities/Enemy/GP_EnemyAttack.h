#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_EnemyAttack.generated.h"

class UGameplayEffect;
struct FGameplayEventData;

UCLASS()
class PROJECT_EDEN_API UGP_EnemyAttack : public UGP_SkillBase
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
	// 정밀한 타격 타이밍이 필요할 때 몽타주 노티파이에서 이 태그를 보낸다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	FGameplayTag AttackEventTag;

	// true면 Gameplay Event 시점에 판정을 내고, false면 어빌리티 시작 즉시 판정한다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Mechanics")
	bool bUseGameplayEventForHitTiming = true;

	/** 적 특유의 수직 오프셋 */
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float HitBoxElevationOffset = 40.0f;

private:
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);

	void PerformAttackHit();

	bool bHasAppliedAttackHit = false;
};
