#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GameplayTagContainer.h"
#include "GP_Skill_WaterPuddle.generated.h"

class AGP_WaterPuddle;
class UGameplayEffect;

/**
 * 물 장판 스킬: 쿨타임 태그 유무에 따라 [스폰] 또는 [당겨오기] 동작 수행
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_WaterPuddle : public UGP_SkillBase
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 동일한 스킬이 여러 슬롯에 있을 때 쿨타임을 공유할지 여부 (false면 슬롯별로 독립 작동) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Settings")
	bool bShareAcrossSlots = false;

protected:
	// --- 수치 및 설정 (Values) ---

	/** 스폰할 물 장판 클래스 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Puddle")
	TSubclassOf<AGP_WaterPuddle> PuddleClass;

	/** 최대 사거리 (스폰/당기기 공통) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Values")
	float MaxTargetDistance = 1500.0f;

	/** 장판이 당겨져 올 때의 속도 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Values")
	float PullSpeed = 1500.0f;

	/** 쿨타임 상태를 판별할 태그 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Cooldown")
	FGameplayTag CooldownTag;

	/** 장판 스폰 시 적용할 수동 쿨타임 이펙트 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Cooldown")
	TSubclassOf<UGameplayEffect> ManualCooldownEffectClass;

	/** 당기기 동작 전용 몽타주 (스폰은 부모의 SkillMontage 사용) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Visuals")
	TObjectPtr<UAnimMontage> PullMontage;
};
