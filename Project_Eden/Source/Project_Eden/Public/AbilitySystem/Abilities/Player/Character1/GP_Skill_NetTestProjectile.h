// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "GP_Skill_NetTestProjectile.generated.h"

class AGP_Projectile;

/**
 * 네트워크 테스트 프로젝타일 스킬
 */
UCLASS()
class PROJECT_EDEN_API UGP_Skill_NetTestProjectile : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	// --- 수치 및 설정 (Values) ---

	// 프로젝타일 클래스 (블루프린트에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	TSubclassOf<AGP_Projectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnForwardOffset = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Projectile")
	float SpawnUpOffset = 60.f;
};
