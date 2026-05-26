#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "GP_TestSkillSet.generated.h"

class UGP_SkillData;

USTRUCT(BlueprintType)
struct FGP_TestSkillPreset
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	FText PresetName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UGP_SkillData> Slot01Skill;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TObjectPtr<UGP_SkillData> Slot02Skill;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_TestSkillSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skill")
	TArray<FGP_TestSkillPreset> Presets;
};
