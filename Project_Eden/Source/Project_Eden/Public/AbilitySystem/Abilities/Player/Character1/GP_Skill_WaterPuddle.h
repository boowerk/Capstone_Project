#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/Player/GP_Skill.h" // 프로젝트 경로에 맞게 수정
#include "GameplayTagContainer.h"
#include "GP_Skill_WaterPuddle.generated.h"

class AGP_WaterPuddle;
class UGameplayEffect;

UCLASS()
class PROJECT_EDEN_API UGP_Skill_WaterPuddle : public UGP_Skill
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// 스폰할 물 장판 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Puddle")
	TSubclassOf<AGP_WaterPuddle> PuddleClass;

	// 마우스 시선 방향으로 최대 몇 거리까지 스폰/당길 수 있는지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Puddle")
	float MaxTargetDistance = 1500.0f;

	// 장판이 당겨져 올 때의 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Puddle")
	float PullSpeed = 1500.0f;

	// --- 쿨타임 수동 제어 ---
	// 이 태그가 내 몸에 있으면 '쿨타임 중'으로 간주하고 당겨오기(Passive) 발동
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Cooldown")
	FGameplayTag CooldownTag;

	// 장판 스폰 시 내 몸에 적용할 쿨타임 부여용 이펙트 (위 태그를 포함해야 함)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Cooldown")
	TSubclassOf<UGameplayEffect> ManualCooldownEffectClass;

	// 스폰용/당기기용 몽타주 분리 (선택 사항)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Animation")
	TObjectPtr<UAnimMontage> SpawnMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Animation")
	TObjectPtr<UAnimMontage> PullMontage;
};
