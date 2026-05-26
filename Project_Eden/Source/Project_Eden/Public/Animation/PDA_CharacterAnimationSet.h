#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "PDA_CharacterAnimationSet.generated.h"

class UAnimMontage;
class USkeletalMesh;

USTRUCT(BlueprintType)
struct FGPDirectionalMovementSpeedProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float NormalForwardSpeed = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float NormalSideSpeed = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float NormalBackSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float SprintForwardSpeed = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float SprintSideSpeed = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.0"))
	float SprintBackSpeed = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed", meta = (ClampMin = "0.01"))
	float MovementSpeedScaleRatio = 1.0f;
};

USTRUCT(BlueprintType)
struct FGPRetargetVisualScaleProfile
{
	GENERATED_BODY()

	// Desired uniform CharacterMesh0 scale in the character/capsule space.
	// If UEFNSourceMesh is scaled, runtime compensates the child-relative scale.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Retarget", meta = (ClampMin = "0.01"))
	float CharacterMeshScale = 1.0f;

	// Desired uniform UEFNSourceMesh scale in the character/capsule space.
	// Keep this separate from MovementSpeedScaleRatio; this is visual/retarget scale only.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Retarget", meta = (ClampMin = "0.01"))
	float UEFNSourceMeshScale = 1.0f;
};

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

	// Runtime motion matching owns locomotion and air loops. Add a jump animation montage here if jump actions need one later.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> DashMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> LightAttackMontages; // Light combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> HeavyAttackMontages; // Heavy combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourceDashMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourcePrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceLightAttackMontages; // Source skeleton fallback order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceHeavyAttackMontages; // Source skeleton fallback order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	float SourceRootMotionTranslationYawOffset = -90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed")
	FGPDirectionalMovementSpeedProfile MovementSpeedProfile;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Retarget")
	FGPRetargetVisualScaleProfile RetargetVisualScaleProfile;
};
