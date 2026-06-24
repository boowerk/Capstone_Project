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
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// Global runtime switch for all skill debug drawing. Console: `g.DrawSkillDebug 0` to suppress, `1` to allow.
	// Skill-spawned actors that own a bDrawDebugs copy also consult this so video capture can mute every debug draw at once.
	static bool IsSkillDebugDrawEnabled();

protected:
	// =========================================================================
	// 2. 디버깅 설정 변수군 (Debug Settings)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugs = false;

	// Per-ability bDrawDebugs AND the global g.DrawSkillDebug cvar must both be on before any debug shape is drawn.
	bool ShouldDrawDebug() const;
};
