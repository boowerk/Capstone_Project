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

UENUM(BlueprintType)
enum class EGPPrimaryAttackType : uint8
{
	Light,
	Heavy
};

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false) override;
	

	// === Locomotion ===
	void SetSprinting(bool bShouldSprint);
	bool IsSprinting() const { return bIsSprinting; }
	bool TryPerformRoll();
	bool IsRolling() const { return bIsRolling; }
	bool IsSprintExitControlLocked() const { return false; }
	UAnimMontage* GetSprintEnterLeftMontage() const { return nullptr; }
	UAnimMontage* GetSprintEnterRightMontage() const { return nullptr; }
	UAnimMontage* GetSprintExitLeftMontage() const { return nullptr; }
	UAnimMontage* GetSprintExitRightMontage() const { return nullptr; }

	// === Combat ===
	void RequestPrimaryAttack(EGPPrimaryAttackType AttackType);
	UAnimMontage* StartPrimaryAttackCombo();
	UAnimMontage* AdvancePrimaryAttackCombo();
	void FinishPrimaryAttackCombo();
	void CancelPrimaryAttackCombo();
	void SetPrimaryAttackActive(bool bIsActive);
	bool IsPrimaryAttacking() const { return bIsPrimaryAttacking; }

	// === Animation Getters ===
	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UBlendSpace* GetLocomotionBlendSpace() const;
	UAnimSequenceBase* GetJumpLoopAnimation() const;
	UAnimMontage* GetLandingMontage() const;
	UAnimMontage* GetRollMontage() const;
	UAnimMontage* GetPrimaryAttackMontage() const;
	
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void ApplySprintStopLock(float LockTime = 0.2f);
	

private:
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	bool bIsLockOn = false; 
	
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	TObjectPtr<AActor> TargetActor; 
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	float LockOnRotationInterpSpeed = 10.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	TObjectPtr<UPDA_WeaponItemCollection> DefaultWeaponCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	FName DefaultWeaponId = TEXT("WP_Common_Fire_Sword");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;

	// === Movement Settings ===
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed")
	float WalkSpeed = 210.0f; 

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed")
	float SprintSpeed = 420.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Roll")
	float RollCooldown = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Attack")
	float PrimaryAttackComboGraceTime = 0.45f;

	// 내부 상태 변수 (대폭 다이어트 됨)
	void ApplyGroundMovementSpeed();
	UAnimMontage* GetPrimaryAttackMontageForStep(EGPPrimaryAttackType AttackType, int32 ComboIndex) const;

	bool bIsRolling = false;
	bool bIsPrimaryAttacking = false;
	bool bIsSprinting = false;
	double SprintExitLockExpireTime = 0.0;

	bool bHasQueuedPrimaryAttackCombo = false;
	EGPPrimaryAttackType RequestedPrimaryAttackType = EGPPrimaryAttackType::Light;
	EGPPrimaryAttackType ActivePrimaryAttackType = EGPPrimaryAttackType::Light;
	int32 PrimaryAttackComboIndex = INDEX_NONE;
	double PrimaryAttackComboExpireTime = 0.0;
	double NextRollAllowedTime = 0.0;
};
