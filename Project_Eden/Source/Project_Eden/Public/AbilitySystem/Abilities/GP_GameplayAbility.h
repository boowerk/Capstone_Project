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

protected:
	// =========================================================================
	// 2. 디버깅 설정 변수군 (Debug Settings)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugs = false;
};
