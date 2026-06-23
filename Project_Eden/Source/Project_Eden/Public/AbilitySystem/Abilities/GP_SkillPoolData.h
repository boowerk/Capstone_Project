#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GP_SkillPoolData.generated.h"

class UGP_SkillData;

UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_SkillPoolData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TArray<TObjectPtr<UGP_SkillData>> Skills;

	UFUNCTION(BlueprintPure, Category = "Skill")
	bool ContainsSkill(const UGP_SkillData* SkillData) const;

	UFUNCTION(BlueprintPure, Category = "Skill")
	TArray<UGP_SkillData*> GetSkills() const;
};
