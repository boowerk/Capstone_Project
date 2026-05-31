#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GP_SkillData.generated.h"

class AActor;
class UNiagaraSystem;

USTRUCT(BlueprintType, meta = (DisplayName = "Element Visual Override"))
struct FGP_ElementVisualActorEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Element", Categories = "GPTags.Tech.Element", DisplayPriority = "1"))
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Impact Actor", ToolTip = "Element-specific visual actor spawned on impact, burst, or completion.", DisplayPriority = "2"))
	TSubclassOf<AActor> VisualActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Active Niagara", ToolTip = "Element-specific Niagara override used by the spawned execution actor while active. If empty, the actor keeps its default VFX.", DisplayPriority = "3"))
	TObjectPtr<UNiagaraSystem> ProjectileVisualSystem;
};

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Element Visual Overrides", TitleProperty = "ElementTag", ToolTip = "Element-specific visual overrides for impact visuals and active execution VFX."))
	TArray<FGP_ElementVisualActorEntry> ElementVisualActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Actor", meta = (DisplayName = "Execution Actor Class", ToolTip = "Actor spawned to execute this skill. Projectile, mine, field, trap, or other runtime actor. If empty, the ability fallback actor is used."))
	TSubclassOf<AActor> SpawnActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float BaseSpellDamage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Damage")
	float ToughnessDamage = 0.f;

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
