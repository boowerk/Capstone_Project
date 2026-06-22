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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TArray<TObjectPtr<UAnimMontage>> HitMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Combat")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;
};
