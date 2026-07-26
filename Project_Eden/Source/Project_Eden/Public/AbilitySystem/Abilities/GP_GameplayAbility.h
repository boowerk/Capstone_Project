#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GP_GameplayAbility.generated.h"

/**
 * UGP_GameplayAbility
 *
 * 프로젝트 Eden의 모든 게임플레이 어빌리티를 위한 베이스 클래스
 */
UCLASS()
class PROJECT_EDEN_API UGP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 기본 오버라이드 함수군 (Base Overrides)
	// =========================================================================
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Global runtime opt-in for skill debug drawing. Console: `g.DrawSkillDebug 1` to allow in non-Shipping builds.
	// Shipping builds always return false, and skill-spawned actors consult the same gate.
	static bool IsSkillDebugDrawEnabled();

protected:
	// =========================================================================
	// 2. 디버깅 설정 변수군 (Debug Settings)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugs = false;

	// Per-ability bDrawDebugs AND the global opt-in must both be on before any debug shape is drawn.
	bool ShouldDrawDebug() const;
};
