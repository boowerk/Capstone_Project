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
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	bool TryPerformRoll();
	bool IsRolling() const { return bIsRolling; }
	bool IsPrimaryAttacking() const { return bIsPrimaryAttacking; }
	bool IsSprintExitControlLocked() const { return bIsSprintExitControlLocked; }
	void SetSprinting(bool bShouldSprint);
	void SetPrimaryAttackActive(bool bIsActive);
	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UBlendSpace* GetLocomotionBlendSpace() const;
	UAnimSequenceBase* GetJumpLoopAnimation() const;
	UAnimMontage* GetLandingMontage() const;
	UAnimMontage* GetRollMontage() const;
	UAnimMontage* GetPrimaryAttackMontage() const;
	UAnimMontage* GetSprintEnterLeftMontage() const;
	UAnimMontage* GetSprintEnterRightMontage() const;
	UAnimMontage* GetSprintExitLeftMontage() const;
	UAnimMontage* GetSprintExitRightMontage() const;
	

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true"))
	float WalkSpeed = 150.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true"))
	float SprintSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float SprintExitMinSpeedRatio = 0.98f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintExitSlideDistance = 80.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float SprintExitControlLockRatio = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintEnterMarkerWindow = 0.08f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintEnterMaxMarkerWaitTime = 0.16f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintTransitionInterruptBlendTime = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Speed", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "1.0"))
	float SprintEnterSpeedRampRatio = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Debug", meta = (AllowPrivateAccess = "true"))
	bool bDebugSprintMarkerPhase = false;

	double NextRollAllowedTime = 0.0;

	void UpdateLandingAnimation(float DeltaSeconds);
	void UpdateRollState();
	void UpdatePendingSprintEnter(float DeltaSeconds);
	void UpdateSprintSpeedTransition(float DeltaSeconds);
	void StartSprintSpeedTransition(bool bShouldSprint, bool bBypassSprintEnterMarkerGate = false);
	void FinishSprintSpeedTransition();
	void StopActiveSprintTransitionMontage();
	bool HasReachedSprintExitSpeed() const;
	bool ShouldDelaySprintEnterForMarker() const;
	bool IsSprintEnterMarkerAligned() const;
	bool IsIdleSprintStart() const;
	UAnimMontage* SelectSprintEnterMontage() const;
	UAnimMontage* SelectSprintExitMontage() const;
	UAnimMontage* SelectSprintTransitionMontageByPlant(UAnimMontage* LeftMontage, UAnimMontage* RightMontage) const;
	void ClearPendingSprintEnter();
	void LogSprintMarkerPhase(UAnimMontage* SelectedMontage) const;
	void ApplyGroundMovementSpeed();
	void FinishRoll();

	bool bIsRolling = false;
	bool bIsPrimaryAttacking = false;
	bool bIsSprinting = false;
	bool bIsSprintEnterPending = false;
	bool bIsSprintSpeedTransitionActive = false;
	bool bIsSprintExitTransitionActive = false;
	bool bIsSprintExitControlLocked = false;
	float SprintEnterPendingElapsedTime = 0.0f;
	float SprintSpeedTransitionStart = 150.0f;
	float SprintSpeedTransitionTarget = 150.0f;
	float SprintSpeedTransitionElapsedTime = 0.0f;
	float SprintSpeedTransitionDuration = 0.0f;
	float SprintSpeedRampDuration = 0.0f;
	FVector SprintExitSlideDirection = FVector::ZeroVector;
	float ActiveLandingElapsedTime = 0.0f;
	TWeakObjectPtr<UAnimMontage> ActiveLandingMontage;
	TWeakObjectPtr<UAnimMontage> ActiveRollMontage;
	TWeakObjectPtr<UAnimMontage> ActiveSprintTransitionMontage;
	
};

