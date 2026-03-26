#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GP_GameplayAbility.generated.h"

UCLASS()
/**
 * UGP_GameplayAbility
 * 프로젝트의 모든 게임플레이 어빌리티를 위한 베이스 클래스
 * bDrawDebugs 
 */
class PROJECT_EDEN_API UGP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
public:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle, 
		const FGameplayAbilityActorInfo* ActorInfo, 
		const FGameplayAbilityActivationInfo ActivationInfo, 
		const FGameplayEventData* TriggerEventData) override;
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Debug")
	bool bDrawDebugs = false;
};
