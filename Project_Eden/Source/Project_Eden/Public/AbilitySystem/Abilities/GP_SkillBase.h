#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_SkillBase.generated.h"

class UGP_SkillData;

/**
 * 프로젝트 Eden의 모든 '스킬'들이 상속받을 베이스 클래스
 * [철학] 행동 메커니즘은 C++에서, 수치는 블루프린트에서 설정
 */
UCLASS()
class PROJECT_EDEN_API UGP_SkillBase : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_SkillBase();

protected:
	// --- 블루프린트에서 설정할 수치들 (Data Driven) ---
	
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Visuals")
	TObjectPtr<UAnimMontage> SkillMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Skill|Values")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** 공격 반경 */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Values")
	float AttackRadius = 200.f;

	/** 전방 오프셋 (캐릭터 앞쪽 어디서 판정을 시작할지) */
	UPROPERTY(EditDefaultsOnly, Category = "Skill|Values")
	float ForwardOffset = 100.f;

	// --- C++ 공통 메커니즘 (Mechanics) ---

	/** 범위 내 적들을 찾아 데미지를 입히는 공통 유틸리티 */
	UFUNCTION(BlueprintCallable, Category = "Skill|Mechanics")
	void PerformAreaAttack();

	/** 어빌리티 종료 시 호출할 정리 함수 */
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
};
