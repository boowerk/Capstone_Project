#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GP_SkillBase.generated.h"

class AActor;
class UGP_SkillData;

/**
 * 프로젝트 Eden의 모든 '스킬'들이 상속받을 베이스 클래스
 * 행동 메커니즘은 C++에서, 수치는 블루프린트에서 설정
 */
UCLASS()
class PROJECT_EDEN_API UGP_SkillBase : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_SkillBase();

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// 블루프린트에서 설정 수치
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Cooldown")
	TSubclassOf<UGameplayEffect> GenericCooldownEffectClass;

	// 공격 반경
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float AttackRadius = 200.f;

	// 전방 오프셋 
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float ForwardOffset = 100.f;

	// C++ 공통 메커니즘 (Mechanics)

	//범위 내 적들을 찾아 데미지를 입히는 공통 유틸리티
	UFUNCTION(BlueprintCallable, Category = "GAS|Skill|Mechanics")
	void PerformAreaAttack();

	FGameplayTag GetCurrentTechElementTag(const FGameplayAbilityActorInfo* ActorInfo) const;
	TSubclassOf<AActor> GetSkillVisualActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackVisualActorClass, FGameplayTag ElementTag = FGameplayTag()) const;
	void SpawnVisualActor(AActor* InstigatorActor, TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator) const;

	// 어빌리티 종료 시 호출할 정리 함수
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UGP_SkillData* GetSkillDataFromSpec(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;
};
