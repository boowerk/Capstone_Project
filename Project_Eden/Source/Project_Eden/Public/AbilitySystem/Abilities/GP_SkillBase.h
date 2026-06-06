#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "GP_SkillBase.generated.h"

class AActor;
class UGP_SkillData;
class UNiagaraSystem;

/**
 * UGP_SkillBase
 *
 * 프로젝트 Eden의 모든 '스킬'들이 상속받을 베이스 클래스
 * 행동 메커니즘은 C++에서 정의하고, 수치 및 에셋은 블루프린트에서 설정합니다.
 */
UCLASS()
class PROJECT_EDEN_API UGP_SkillBase : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 라이프사이클 및 기본 오버라이드 함수군 (Lifecycle & Overrides)
	// =========================================================================
	UGP_SkillBase();

	virtual bool CheckCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual void ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// =========================================================================
	// 2. C++ 공통 스킬 메커니즘 함수군 (Skill Mechanics Helpers)
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "GAS|Skill|Mechanics")
	void PerformAreaAttack();

	FGameplayTag GetCurrentTechElementTag(const FGameplayAbilityActorInfo* ActorInfo) const;
	float GetSkillAugmentRadiusMultiplier(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const;
	float GetSkillAugmentRangeMultiplier(const UGP_SkillData* SkillData, const FGameplayAbilityActorInfo* ActorInfo) const;
	TSubclassOf<AActor> GetSkillVisualActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackVisualActorClass, FGameplayTag ElementTag = FGameplayTag(), FGameplayTag CueTag = FGameplayTag()) const;
	TSubclassOf<AActor> GetSkillSpawnActorClass(const UGP_SkillData* SkillData, TSubclassOf<AActor> FallbackActorClass) const;
	UNiagaraSystem* GetSkillNiagaraSystem(const UGP_SkillData* SkillData, FGameplayTag ElementTag = FGameplayTag(), FGameplayTag CueTag = FGameplayTag()) const;
	UNiagaraSystem* GetProjectileVisualSystem(const UGP_SkillData* SkillData, FGameplayTag ElementTag = FGameplayTag()) const;
	void SpawnVisualActor(AActor* InstigatorActor, TSubclassOf<AActor> VisualActorClass, const FVector& Location, const FRotator& Rotation = FRotator::ZeroRotator, float VisualScale = 1.0f) const;
	UGP_SkillData* GetSkillDataFromSpec(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	// =========================================================================
	// 3. 애니메이션 및 콜백 함수군 (Animation & Callbacks)
	// =========================================================================
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// =========================================================================
	// 4. 스킬 설정 및 수치 변수군 (Skill Assets & Values)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Visuals")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Data", meta = (DisplayName = "Default Skill Data", ToolTip = "Fallback data asset used when this ability was granted without a UGP_SkillData SourceObject. Runtime-equipped skills still prefer the spec SourceObject."))
	TObjectPtr<UGP_SkillData> DefaultSkillData;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Cooldown")
	TSubclassOf<UGameplayEffect> GenericCooldownEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float AttackRadius = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Skill|Values")
	float ForwardOffset = 100.f;
};
