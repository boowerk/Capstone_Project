// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"

#include "GP_Primary.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT_EDEN_API UGP_Primary : public UGP_GameplayAbility
{
	GENERATED_BODY()
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities|Attack")
	TObjectPtr<UAnimMontage> AttackMontage; // 이 변수 세팅시 자동 재생, 비워두면 BP에서 조작
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Abilities|Attack")
	FGameplayTag AttackEventTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Effects")
	TSubclassOf<UGameplayEffect> DamageEffectClass;
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxRadius = 100.0f;	
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxForwardOffset = 200.0f;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Abilities")
	float HitBoxElevationOffset = 20.0f;
	
	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnAttackEventReceived(FGameplayEventData Payload);
};
