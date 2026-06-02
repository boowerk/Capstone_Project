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

/** Bow 관련 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPBowMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimNeutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Notch;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> RapidShootLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Shoot;
};

/** 회피 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPDodgeMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Left;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Left_RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Right;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Right_RM;
};

/** 총기 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPGunMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimDown;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimNeutral;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> AimUp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> IdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Reload;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Shoot;
};

/** 피격 반응 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPHitReactionMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Chest;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Head;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Stomach;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ShoulderL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ShoulderR;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Knockback;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Knockback_RM;
};

/** 공중 띄우기 콤보 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPLiftAirMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> IdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> HitL;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> HitR;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FallLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Fall_RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> FallImpact;
};

/** 격투 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPMeleeMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Combo;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Hook;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> HookRec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Knee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> KneeRec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Uppercut;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> PunchCross;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> PunchJab;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Kick;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> PunchKickEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> PunchKickExit;
};

/** 검 기본 상태 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPSwordBaseMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Enter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Exit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Idle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Block;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Dash_RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> GroundPound_RM;
};

/** 검 공중 콤보 몽타주 세트. UpperCut_RM 으로 적을 띄운 뒤 Aerial 공격으로 이어진다. */
USTRUCT(BlueprintType)
struct FGPSwordAerialMontages
{
	GENERATED_BODY()

	/** 공중 띄우기 진입 어퍼컷 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> UpperCut_RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> IdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Aerial_A;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Aerial_A_Rec;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Aerial_B;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> ComboLoop;
};

/** 구르기 몽타주 세트. Dash 어빌리티는 Roll_RM 을 사용한다. */
USTRUCT(BlueprintType)
struct FGPRollMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Roll;

	/** 루트모션 구르기. Dash 어빌리티 기본 재생 몽타주. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Roll_RM;
};

/** 방패 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPShieldMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> IdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Break;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> OneShot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> Dash_RM;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> SprintLoop;
};

/** 마법 몽타주 세트 */
USTRUCT(BlueprintType)
struct FGPSpellMontages
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> SimpleEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> SimpleExit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> SimpleIdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> SimpleShoot;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DoubleEnter;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DoubleExit;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DoubleIdleLoop;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UAnimMontage> DoubleShootLoop;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UPDA_CharacterAnimationSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------------
	// Animation | Visual
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TObjectPtr<USkeletalMesh> CharacterMesh;

	/** 이 캐릭터가 사용할 애니메이션 블루프린트 클래스 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Visual")
	TSubclassOf<UAnimInstance> AnimBlueprintClass;

	// -----------------------------------------------------------------------
	// Animation | Action
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> BackFlipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPBowMontages BowMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> DeathMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPDodgeMontages DodgeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPGunMontages GunMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPHitReactionMontages HitMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> KipUpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPLiftAirMontages LiftAirMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPMeleeMontages MeleeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPSwordBaseMontages SwordMontages;

	// Runtime motion matching owns locomotion and air loops. Add a jump animation montage here if jump actions need one later.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> PrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> LightAttackMontages; // Light combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> HeavyAttackMontages; // Heavy combo order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TArray<TObjectPtr<UAnimMontage>> RegularAttackMontages; // Regular combo order: A, B, C.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPSwordAerialMontages SwordAerialMontages;

	/** 구르기 세트. Dash 어빌리티는 Roll_RM 을 사용한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPRollMontages RollMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPShieldMontages ShieldMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	FGPSpellMontages SpellMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Action")
	TObjectPtr<UAnimMontage> ThrowMontage;

	// -----------------------------------------------------------------------
	// Animation | Runtime Retarget Fallback
	// Action 섹션과 1:1 미러. 리타겟 몽타주가 없을 때 소스 스켈레톤 몽타주로 폴백한다.
	// -----------------------------------------------------------------------

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourceBackFlipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPBowMontages SourceBowMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceDeathMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPDodgeMontages SourceDodgeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPGunMontages SourceGunMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPHitReactionMontages SourceHitMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourceKipUpMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPLiftAirMontages SourceLiftAirMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPMeleeMontages SourceMeleeMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPSwordBaseMontages SourceSwordMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourcePrimaryAttackMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceLightAttackMontages; // Source skeleton fallback order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceHeavyAttackMontages; // Source skeleton fallback order: A, B, C, D.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TArray<TObjectPtr<UAnimMontage>> SourceRegularAttackMontages; // Source skeleton fallback order: A, B, C.

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPSwordAerialMontages SourceSwordAerialMontages;

	/** 구르기(Dash 어빌리티) 소스 스켈레톤 폴백 몽타주 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPRollMontages SourceRollMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPShieldMontages SourceShieldMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	FGPSpellMontages SourceSpellMontages;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	TObjectPtr<UAnimMontage> SourceThrowMontage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Runtime Retarget Fallback")
	float SourceRootMotionTranslationYawOffset = -90.0f;

	// -----------------------------------------------------------------------
	// Movement | Speed
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement|Speed")
	FGPDirectionalMovementSpeedProfile MovementSpeedProfile;

	// -----------------------------------------------------------------------
	// Animation | Retarget
	// -----------------------------------------------------------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation|Retarget")
	FGPRetargetVisualScaleProfile RetargetVisualScaleProfile;
};
