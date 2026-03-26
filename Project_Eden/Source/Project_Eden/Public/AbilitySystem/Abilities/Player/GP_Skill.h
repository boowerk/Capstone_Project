// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_Skill.generated.h"

UCLASS()
class PROJECT_EDEN_API UGP_Skill : public UGP_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UGP_Skill();

protected:
	// 액션 RPG 스킬은 주로 애니메이션 몽타주 재생과 함께 이펙트/데미지를 처리합니다.
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
