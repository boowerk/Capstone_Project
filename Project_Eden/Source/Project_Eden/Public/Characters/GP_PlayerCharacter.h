#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_BaseCharacter.h"
#include "AbilitySystemInterface.h"

#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UPDA_WeaponItemCollection;
class UPDA_CharacterAnimationSet;
class UAnimMontage;
class UAnimSequenceBase;
class UBlendSpace;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	bool TryPerformRoll();
	bool IsRolling() const { return bIsRolling; }
	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UBlendSpace* GetLocomotionBlendSpace() const;
	UAnimSequenceBase* GetJumpLoopAnimation() const;
	UAnimMontage* GetLandingMontage() const;
	UAnimMontage* GetRollMontage() const;
	UAnimMontage* GetPrimaryAttackMontage() const;
	

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	bool bIsLockOn = false; // lockOn 주석 한글로 외안되는거야이야
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	TObjectPtr<AActor> TargetActor; // lockOn Target
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	float LockOnRotationInterpSpeed = 10.0f;// lockOn 카메라 드래그 구현용

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDA_WeaponItemCollection> DefaultWeaponCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon", meta = (AllowPrivateAccess = "true"))
	FName DefaultWeaponId = TEXT("WP_Common_Fire_Sword");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Air", meta = (AllowPrivateAccess = "true"))
	float MinLandingSpeedForMontage = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Air", meta = (AllowPrivateAccess = "true"))
	float MinLandingPlayTimeBeforeBlendOut = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation|Air", meta = (AllowPrivateAccess = "true"))
	float LandingMontageBlendOutTime = 0.15f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll", meta = (AllowPrivateAccess = "true"))
	float RollCooldown = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll", meta = (AllowPrivateAccess = "true"))
	float RollDistance = 350.0f;

	double NextRollAllowedTime = 0.0;

	void UpdateLandingAnimation(float DeltaSeconds);
	void UpdateRollMovement(float DeltaSeconds);
	void FinishRoll();

	FVector ActiveRollDirection = FVector::ZeroVector;
	float ActiveRollDuration = 0.0f;
	float ActiveRollElapsedTime = 0.0f;
	float ActiveRollDistanceTravelled = 0.0f;
	bool bIsRolling = false;
	float ActiveLandingElapsedTime = 0.0f;
	TWeakObjectPtr<UAnimMontage> ActiveLandingMontage;
	TWeakObjectPtr<UAnimMontage> ActiveRollMontage;
	
};

