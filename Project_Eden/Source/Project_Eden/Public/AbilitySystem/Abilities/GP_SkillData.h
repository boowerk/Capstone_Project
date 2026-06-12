#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "GP_SkillData.generated.h"

class AActor;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EGP_SkillVisualType : uint8
{
	Actor,
	Niagara
};

USTRUCT(BlueprintType, meta = (DisplayName = "Skill Visual Cue"))
struct FGP_SkillVisualCueEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Cue Tag", ToolTip = "Situation/context tag for this visual, such as cast, active, impact, tick, or expire. Empty can be used as the default for this visual type.", DisplayPriority = "1"))
	FGameplayTag CueTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Element", Categories = "GPTags.Tech.Element", ToolTip = "Optional element condition. Empty means any element.", DisplayPriority = "2"))
	FGameplayTag ElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Visual Type", DisplayPriority = "3"))
	EGP_SkillVisualType VisualType = EGP_SkillVisualType::Niagara;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Visual Actor", EditCondition = "VisualType == EGP_SkillVisualType::Actor", EditConditionHides, DisplayPriority = "4"))
	TSubclassOf<AActor> VisualActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Niagara System", EditCondition = "VisualType == EGP_SkillVisualType::Niagara", EditConditionHides, DisplayPriority = "5"))
	TObjectPtr<UNiagaraSystem> NiagaraSystem;
};

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

USTRUCT(BlueprintType, meta = (DisplayName = "Periodic Area Niagara Bindings"))
struct FGP_PeriodicAreaNiagaraBindings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual|Niagara", meta = (DisplayName = "Radius Parameter Name", ToolTip = "Optional Niagara float parameter that receives the periodic area's radius. Use the exact User parameter name."))
	FName RadiusParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual|Niagara", meta = (DisplayName = "Duration Parameter Name", ToolTip = "Optional Niagara float parameter that receives the periodic area's duration. Use the exact User parameter name."))
	FName DurationParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual|Niagara", meta = (DisplayName = "Spawn Rate Parameter Name", ToolTip = "Optional Niagara float parameter that receives 1 / damage interval. Use the exact User parameter name."))
	FName SpawnRateParameterName;
};

UENUM(BlueprintType)
enum class EGP_CooldownPolicy : uint8
{
	None,
	Generic,
	Custom
};

UENUM(BlueprintType)
enum class EGP_ProjectileImpactDamageMode : uint8
{
	DirectOnly,
	DirectAndSplash
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill", meta = (Categories = "GPTags.Ability.Skill.Id", ToolTip = "Stable skill identity tag used by augments, unlocks, stats, and other non-cooldown systems."))
	FGameplayTag SkillIdTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual")
	TSubclassOf<AActor> SkillVisualActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Visual Cues", TitleProperty = "CueTag", ToolTip = "Flexible skill visuals selected by situation cue, visual type, and optional element. Use this for cast/active/impact/tick/expire variations."))
	TArray<FGP_SkillVisualCueEntry> VisualCues;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual", meta = (DisplayName = "Element Visual Overrides", TitleProperty = "ElementTag", ToolTip = "Element-specific visual overrides for impact visuals and active execution VFX."))
	TArray<FGP_ElementVisualActorEntry> ElementVisualActorClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Visual|Niagara", meta = (DisplayName = "Periodic Area Parameter Bindings", ToolTip = "Maps periodic area gameplay values to this skill's Niagara User parameters. Empty names are ignored."))
	FGP_PeriodicAreaNiagaraBindings PeriodicAreaNiagaraBindings;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Actor", meta = (DisplayName = "Execution Actor Class", ToolTip = "Actor spawned to execute this skill. Projectile, mine, field, trap, or other runtime actor. If empty, the ability fallback actor is used."))
	TSubclassOf<AActor> SpawnActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile")
	EGP_ProjectileImpactDamageMode ProjectileImpactDamageMode = EGP_ProjectileImpactDamageMode::DirectOnly;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0.0", EditCondition = "ProjectileImpactDamageMode == EGP_ProjectileImpactDamageMode::DirectAndSplash", EditConditionHides))
	float SplashRadius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile", meta = (ClampMin = "0.0", EditCondition = "ProjectileImpactDamageMode == EGP_ProjectileImpactDamageMode::DirectAndSplash", EditConditionHides))
	float SplashDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile|Visual", meta = (DisplayName = "Impact Radius Scale Parameter Name", ToolTip = "Optional Niagara Vector2D User parameter whose authored BP value is multiplied by the final splash radius multiplier. Example: User.SpriteAllSize.", EditCondition = "ProjectileImpactDamageMode == EGP_ProjectileImpactDamageMode::DirectAndSplash", EditConditionHides))
	FName ImpactRadiusScaleParameterName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile|Debug", meta = (EditCondition = "ProjectileImpactDamageMode == EGP_ProjectileImpactDamageMode::DirectAndSplash", EditConditionHides))
	bool bDrawSplashDebug = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile|Multi Hit", meta = (DisplayName = "Projectile Max Total Hits", ClampMin = "1", ToolTip = "Maximum total number of damage hits applied by one multi-hit projectile across all targets."))
	int32 ProjectileMaxHitsPerTarget = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile|Multi Hit", meta = (ClampMin = "0.01", Units = "s", ToolTip = "Delay between reserved hits after a multi-hit projectile touches a target."))
	float ProjectileHitInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Projectile|Multi Hit", meta = (ClampMin = "0.0", ToolTip = "Damage scale applied to each hit of a multi-hit projectile."))
	float ProjectileDamagePerHitMultiplier = 0.4f;

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
