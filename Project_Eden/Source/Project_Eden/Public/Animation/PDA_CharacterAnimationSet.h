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

	/** 이 캐릭터가 사용할 애니메이션 블루프린트 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimSequenceBase> SprintStopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Air")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Air")
	TObjectPtr<UAnimMontage> LandingMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> LightAttackMontages; // Light combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> HeavyAttackMontages; // Heavy combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintEnterLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintEnterRightMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintExitLeftMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimMontage> SprintExitRightMontage;
};
