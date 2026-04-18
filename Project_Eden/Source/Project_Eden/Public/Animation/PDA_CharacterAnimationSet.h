#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "PDA_CharacterAnimationSet.generated.h"

class UAnimMontage;
class UAnimSequenceBase;
class UBlendSpace;
class USkeletalMesh;

UCLASS(BlueprintType)
class PROJECT_EDEN_API UPDA_CharacterAnimationSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TObjectPtr<USkeletalMesh> CharacterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Air")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Air")
	TObjectPtr<UAnimMontage> LandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> RollMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintEnterLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintEnterRightMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintExitLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintExitRightMontage;
};
