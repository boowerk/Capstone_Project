#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PDA_EnemyAnimationSet.generated.h"

class UAnimInstance;
class UAnimMontage;
class UAnimSequence;
class USkeletalMesh;

/**
 * Enemy-only animation data. Keep player weapons, movement abilities, and
 * runtime-retarget fallback data out of this asset.
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UPDA_EnemyAnimationSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TObjectPtr<USkeletalMesh> CharacterMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimSequence> IdleAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Locomotion")
	TObjectPtr<UAnimSequence> JogAnimation;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TArray<TObjectPtr<UAnimMontage>> LightAttackMontages;

	/**
	 * Optional lower-body root-motion clips paired by index with LightAttackMontages.
	 * Leave an entry empty when that attack should remain in-place.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Root Motion")
	TArray<TObjectPtr<UAnimSequence>> LowerBodyRootMotionSequences;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Forward Step")
	bool bUseAbilityForwardStep = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Forward Step", meta = (EditCondition = "bUseAbilityForwardStep", ClampMin = "0.0", Units = "s"))
	float AbilityForwardStepDelaySeconds = 0.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Forward Step", meta = (EditCondition = "bUseAbilityForwardStep", ClampMin = "0.0", Units = "cm"))
	float AbilityForwardStepDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Forward Step", meta = (EditCondition = "bUseAbilityForwardStep", ClampMin = "0.01", Units = "s"))
	float AbilityForwardStepDurationSeconds = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat|Forward Step", meta = (EditCondition = "bUseAbilityForwardStep", ClampMin = "0.0", Units = "cm/s"))
	float AbilityForwardStepPushSpeed = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TArray<TObjectPtr<UAnimMontage>> HitMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;
};
