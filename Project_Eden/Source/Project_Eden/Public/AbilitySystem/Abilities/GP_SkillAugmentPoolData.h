#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GP_SkillAugmentPoolData.generated.h"

class UGP_SkillAugmentData;

UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_SkillAugmentPoolData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill|Augment|Pool")
	TArray<TObjectPtr<UGP_SkillAugmentData>> Augments;

	UFUNCTION(BlueprintCallable, Category = "Skill|Augment|Pool")
	TArray<UGP_SkillAugmentData*> PickRandomAugments(int32 Count) const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Augment|Pool")
	TArray<UGP_SkillAugmentData*> PickRandomAugmentsExcluding(int32 Count, const TArray<UGP_SkillAugmentData*>& ExcludedAugments) const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Augment|Pool")
	TArray<UGP_SkillAugmentData*> PickRandomAugmentsExcludingForSkills(
		int32 Count,
		const TArray<UGP_SkillAugmentData*>& ExcludedAugments,
		FGameplayTagContainer AvailableSkillTags) const;

	UFUNCTION(BlueprintCallable, Category = "Skill|Augment|Pool")
	TArray<UGP_SkillAugmentData*> PickRandomAugmentsExcludingForElement(
		int32 Count,
		const TArray<UGP_SkillAugmentData*>& ExcludedAugments,
		FGameplayTag CurrentElementTag) const;
};
