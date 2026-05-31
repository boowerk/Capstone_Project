#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GP_SkillAugmentData.generated.h"

class AActor;
class UNiagaraSystem;
class UTexture2D;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Target", meta = (Categories = "GPTags.Ability.Skill.Id", ToolTip = "Skill identity tags this augment can affect. Empty means the selection system must decide validity another way."))
	FGameplayTagContainer TargetSkillTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Element", meta = (Categories = "GPTags.Tech.Element", ToolTip = "Element this augment grants or prefers. Empty means no element change."))
	FGameplayTag GrantedElementTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Modifier")
	FGP_SkillAugmentNumericModifiers Modifiers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Visual", meta = (DisplayName = "Impact Visual Actor Override", ToolTip = "Optional impact/burst/completion visual override supplied by this augment."))
	TSubclassOf<AActor> ImpactVisualActorOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Visual", meta = (DisplayName = "Active VFX Override", ToolTip = "Optional active Niagara override supplied by this augment."))
	TObjectPtr<UNiagaraSystem> ActiveVFXOverride;
};
