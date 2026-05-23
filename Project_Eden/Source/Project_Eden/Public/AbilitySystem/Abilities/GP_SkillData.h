#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GP_SkillData.generated.h"

class AActor;

UENUM(BlueprintType)
enum class EGP_CooldownPolicy : uint8
{
	None,
	Generic,
	Custom
};

/**
 * 개별 스킬의 정보를 담는 데이터 에셋
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_SkillData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText SkillName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText SkillDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSoftObjectPtr<UTexture2D> SkillIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TSubclassOf<UGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
	TSubclassOf<AActor> SkillVisualActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float BaseSpellDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float ToughnessDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage", meta = (Categories = "GPTags.Damage.Element"))
	FGameplayTag PrimaryDamageElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage|Coefficient")
	float AttackPowerCoefficient = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage|Coefficient")
	float MagicPowerCoefficient = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage|Coefficient")
	float DefenseCoefficient = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage|Coefficient")
	float MaxHealthCoefficient = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cooldown")
	EGP_CooldownPolicy CooldownPolicy = EGP_CooldownPolicy::Generic;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cooldown", meta = (EditCondition = "CooldownPolicy == EGP_CooldownPolicy::Generic", EditConditionHides))
	FGameplayTag CooldownTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Cooldown", meta = (ClampMin = "0.0", EditCondition = "CooldownPolicy == EGP_CooldownPolicy::Generic", EditConditionHides))
	float CooldownDuration = 0.f;

	/** 이 스킬이 장착될 수 있는 기본 권장 슬롯 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FGameplayTag DefaultSlotTag;

	/** 이 스킬이 장착 가능한 모든 슬롯 리스트 (비어있으면 어디든 가능) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Restriction")
	FGameplayTagContainer SupportedSlotTags;
};
