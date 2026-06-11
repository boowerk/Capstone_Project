#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "VFX/GP_NiagaraParameterOverride.h"
#include "GP_SkillAugmentData.generated.h"

class AActor;
class UNiagaraSystem;
class UTexture2D;

UENUM(BlueprintType)
enum class EGP_SkillAugmentType : uint8
{
	BasicAttack UMETA(DisplayName = "Basic Attack"),
	Skill UMETA(DisplayName = "Skill"),
	Ultimate UMETA(DisplayName = "Ultimate"),
	Passive UMETA(DisplayName = "Passive")
};

USTRUCT(BlueprintType)
struct FGP_SkillAugmentNumericModifiers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier", meta = (ClampMin = "0.0", ToolTip = "Final damage multiplier applied by this augment. 1.0 keeps base damage."))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier", meta = (ClampMin = "0.0", ToolTip = "Cooldown multiplier applied by this augment. 1.0 keeps base cooldown, 0.8 means 20% shorter cooldown."))
	float CooldownMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier", meta = (ClampMin = "0.0", ToolTip = "Area/radius multiplier applied by this augment. 1.0 keeps base radius."))
	float RadiusMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier", meta = (ClampMin = "0.0", ToolTip = "Range/distance multiplier applied by this augment. 1.0 keeps base range."))
	float RangeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier", meta = (ClampMin = "0", ToolTip = "Extra projectile count granted by this augment. 0 keeps base count."))
	int32 ProjectileCountBonus = 0;
};

USTRUCT(BlueprintType)
struct FGP_SkillAugmentPeriodicAreaDamage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay")
	bool bEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay", meta = (EditCondition = "bEnabled", EditConditionHides, ClampMin = "0.0", Units = "cm"))
	float Radius = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay", meta = (EditCondition = "bEnabled", EditConditionHides, ClampMin = "0.0", Units = "s"))
	float Duration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay", meta = (EditCondition = "bEnabled", EditConditionHides, ClampMin = "0.01", Units = "s"))
	float Interval = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay", meta = (EditCondition = "bEnabled", EditConditionHides, ClampMin = "0.0", ToolTip = "Damage multiplier applied on each periodic tick."))
	float DamagePerTickMultiplier = 0.25f;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_SkillAugmentData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Info", meta = (ToolTip = "Optional stable tag/id for this augment."))
	FGameplayTag AugmentTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Info")
	FText AugmentName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Info", meta = (MultiLine = "true"))
	FText AugmentDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Info")
	TSoftObjectPtr<UTexture2D> AugmentIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Info")
	EGP_SkillAugmentType AugmentType = EGP_SkillAugmentType::Skill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Target", meta = (Categories = "GPTags.Ability.Skill.Id", ToolTip = "Skill identity tags this augment can affect. Empty means every skill can use this augment."))
	FGameplayTagContainer TargetSkillTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Element", meta = (Categories = "GPTags.Tech.Element", ToolTip = "Deprecated. Element requirements are ignored; use TargetSkillTags for skill-specific augments."))
	FGameplayTag RequiredElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Element", meta = (Categories = "GPTags.Tech.Element", ToolTip = "Deprecated. Granted elements are ignored; use skill selection and SkillIdTag instead."))
	FGameplayTag GrantedElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier")
	FGP_SkillAugmentNumericModifiers Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Gameplay")
	FGP_SkillAugmentPeriodicAreaDamage PeriodicAreaDamage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Visual", meta = (DisplayName = "Impact Visual Actor Override", ToolTip = "Optional impact/burst/completion visual override supplied by this augment."))
	TSubclassOf<AActor> ImpactVisualActorOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Visual", meta = (DisplayName = "Active VFX Override", ToolTip = "Optional active Niagara override supplied by this augment."))
	TObjectPtr<UNiagaraSystem> ActiveVFXOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Visual", meta = (ToolTip = "Niagara User Parameters applied when this augment's target skill spawns a visual actor. Enter the exact parameter name, for example User.SpawnCount."))
	TArray<FGP_NiagaraParameterOverride> NiagaraParameterOverrides;
};
