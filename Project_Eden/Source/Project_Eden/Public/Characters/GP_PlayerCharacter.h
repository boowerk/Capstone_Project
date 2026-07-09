#pragma once

#include "CoreMinimal.h"
#include "AI/Perception/FootstepNoisePolicy.h"
#include "Characters/GP_BaseCharacter.h"
#include "Animation/PDA_CharacterAnimationSet.h"
#include "AbilitySystemInterface.h"
#include "GameplayTagContainer.h"
#include "GP_PlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class AGP_WhiteVoidSetActor;
class USkeletalMeshComponent;
class UAnimInstance;
class UAnimMontage;
class UPDA_WeaponItemCollection;
class UPDA_CharacterAnimationSet;
class UAnimSequenceBase;
class UBlendSpace;
class UGP_SkillData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSkillEquipped, FGameplayTag, SlotTag, UGP_SkillData*, SkillData);
DECLARE_MULTICAST_DELEGATE(FOnActionRootMotionCancelInput);

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	// =========================================================================
	// 1. 라이프사이클 & 기본 오버라이드 함수군 (Lifecycle & Base Overrides)
	// =========================================================================
	AGP_PlayerCharacter();
	virtual ~AGP_PlayerCharacter() override;
	virtual void BeginPlay() override;
	virtual void PostInitializeComponents() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false) override;

	// =========================================================================
	// 2. GAS (Gameplay Ability System) 인터페이스 & 상태 초기화
	// =========================================================================
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

	// =========================================================================
	// 3. 이동 및 상태 제어 API (Movement & State Control)
	// =========================================================================
	void ToggleSprinting();
	void StartSprinting();
	void StopSprinting();
	bool IsSprinting() const;
	bool IsPrimaryAttackInProgress() const { return bPrimaryAttackInProgress; }
	void SetPrimaryAttackInProgress(bool bInProgress);

	bool TryPerformDash();
	bool IsDashing() const;
	bool IsLockOn() const { return bIsLockOn; }
	bool AimPrimaryAttackAtBestTarget(float SearchRadius, float ForwardOffset, float Duration);

	// =========================================================================
	// 4. 화이트 보이드 API (White Void Transition)
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "White Void")
	void ToggleWhiteVoid();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void EnterWhiteVoid();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void ExitWhiteVoid();

	UFUNCTION(BlueprintPure, Category = "White Void")
	bool IsInWhiteVoid() const { return bIsInWhiteVoid; }

	// =========================================================================
	// 5. 이동 속도 프로필 & 제어 API (Movement Speed & Profiles)
	// =========================================================================
	float GetMovementSpeedScaleRatio() const;
	float GetScaledNormalWalkSpeed() const;
	float GetScaledSprintSpeed() const;
	float ResolveDirectionalMoveSpeed(const FVector2D& MoveInput, bool bSprinting) const;
	const FGPDirectionalMovementSpeedProfile& GetActiveMovementSpeedProfile() const;

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetGASMovementSpeedMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintPure, Category = "Movement|Speed")
	float GetGASMovementSpeedMultiplier() const { return GASMovementSpeedMultiplier; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetGASMovementSpeedScaleRatioMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetMovementSpeedProfileOverride(const FGPDirectionalMovementSpeedProfile& NewProfile);

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void ClearMovementSpeedProfileOverride();

	// =========================================================================
	// 6. 애니메이션 세트 & UEFN 몽타주 / 루트 모션 / 액션 관성 제어
	// =========================================================================
	virtual void UpdateAnimationSet() override;
	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UAnimInstance* GetUEFNSourceAnimInstance() const;

	float PlayUEFNSourceFallbackMontage(UAnimMontage* Montage, float PlayRate = 1.0f, float PreviousMontageBlendOutTime = 0.1f);
	bool IsPlayingUEFNSourceFallbackMontage() const;
	void StopUEFNSourceFallbackMontage(float BlendOutTime = 0.2f);
	void ClearUEFNSourceFallbackMontageState();

	void SetActionRootMotionInputCancelEnabled(bool bEnabled);
	bool RequestActionRootMotionCancelIfMovementHeld();
	void ClearActionRootMotionCancelMovementInput();

	void BeginActionMotionTracking();
	void StopActionMotionTracking();
	void ApplyCurrentActionInertia();
	void BeginPrimaryAttackMovementAssist(float SpeedRatio);
	void StopPrimaryAttackMovementAssist();

	FVector GetLastUEFNSourceRootMotionVelocity() const { return LastUEFNSourceRootMotionVelocity; }
	FVector GetCurrentActionMotionVelocity() const { return CurrentActionMotionVelocity; }
	FVector GetActionMotionAnimVelocity() const;
	bool IsUsingPostActionAnimVelocity() const;

	void SetActionLowerBodyMotionMatchBlendEnabled(bool bEnabled);
	void SetActionLowerBodyMotionMatchBlendTargetAlpha(float TargetAlpha);
	bool ShouldBlendActionLowerBodyToMotionMatching() const { return bBlendActionLowerBodyToMotionMatching; }
	float GetActionLowerBodyMotionMatchBlendTargetAlpha() const { return ActionLowerBodyMotionMatchBlendTargetAlpha; }

	FOnActionRootMotionCancelInput OnActionRootMotionCancelInput;

	// =========================================================================
	// 7. 런타임 스킬 장착 (Runtime Skill Equipment)
	// =========================================================================
	UFUNCTION(BlueprintCallable, Category = "GAS|Combat")
	void EquipSkill(UGP_SkillData* NewSkillData, FGameplayTag SlotTag, bool bIgnoreRestrictions = false);

	UFUNCTION(BlueprintCallable, Category = "GAS|Combat", meta = (DeprecatedFunction, DeprecationMessage = "Use DataAsset version instead"))
	void EquipSkillByClass(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass);

	UPROPERTY(BlueprintAssignable, Category = "GAS|Events")
	FOnSkillEquipped OnSkillEquipped;

protected:
	// =========================================================================
	// 8. 가속도 제어 & 무브먼트 튜닝 함수군 (Movement Acceleration & Tuning)
	// =========================================================================
	void UpdateConditionalMaxAcceleration(float DeltaSeconds);
	void UpdateFootstepNoise();
	bool ShouldStartAccelerationClamp() const;
	bool ShouldReleaseAccelerationClamp() const;
	void RestoreNormalMaxAcceleration();

	// =========================================================================
	// 9. 애니메이션 및 비주얼 제어 이벤트 콜백 (Animation & Visual Callbacks)
	// =========================================================================
	void OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void OnFixedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void ApplyMovementSpeedFromAnimationSet();
	void ApplyRetargetVisualScaleFromAnimationSet();
	void UpdateCameraMotion(float DeltaSeconds);
	void RefreshCurrentMaxWalkSpeed();
	void PushMovementSpeedScaleRatioToAnimInstances();

	// =========================================================================
	// 10. 컴뱃 & 록온 (Combat & LockOn Variables)
	// =========================================================================
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	bool bIsLockOn = false;

	// AI hearing stimuli are emitted only by the authoritative player instance to avoid network duplicates.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps")
	bool bEmitFootstepNoise = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Footsteps", meta = (EditCondition = "bEmitFootstepNoise"))
	FGPFootstepNoiseSettings FootstepNoiseSettings;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|LockOn")
	float LockOnRotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Primary Attack", meta = (ClampMin = "0.0"))
	float PrimaryAttackAutoFacingRotationInterpSpeed = 18.0f;

	// =========================================================================
	// 11. 장비 & 무기 관련 설정 (Equipment & Weapon Settings)
	// =========================================================================
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	TObjectPtr<UPDA_WeaponItemCollection> DefaultWeaponCollection;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Equipment|Weapon")
	FName DefaultWeaponId = TEXT("WP_Common_Fire_Sword");

	// =========================================================================
	// 12. 이동 프로필 및 가속도 제어 튜닝 설정값 (Movement Tuning Settings)
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed")
	FGPDirectionalMovementSpeedProfile MovementSpeedProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed")
	bool bOverrideMovementSpeedProfile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed", meta = (EditCondition = "bOverrideMovementSpeedProfile"))
	FGPDirectionalMovementSpeedProfile MovementSpeedProfileOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed|GAS", meta = (ClampMin = "0.01"))
	float GASMovementSpeedMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed|GAS", meta = (ClampMin = "0.01"))
	float GASMovementSpeedScaleRatioMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampMaxAcceleration = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampMaxDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampReleaseSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	bool bDebugStartAccelerationClamp = false;

private:
	// =========================================================================
	// 13. 컴포넌트 선언 (Internal Components)
	// =========================================================================
	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float IdleCameraArmLength = 340.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float NormalCameraArmLength = 380.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SprintCameraArmLength = 460.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraArmLengthInterpSpeed = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float IdleCameraSpeedThreshold = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Composition", meta = (AllowPrivateAccess = "true"))
	FVector CameraSocketOffset = FVector(0.0f, 65.0f, 20.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Composition", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float CameraSocketOffsetInterpSpeed = 8.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Retarget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> UEFNSourceMesh;

	// =========================================================================
	// 14. 화이트 보이드 제어 상태 & 내부 함수 (White Void Internal State & Helpers)
	// =========================================================================
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	FVector WhiteVoidOffset = FVector(0.0, 0.0, -10000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	float WhiteVoidFloorTopZOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	FVector WhiteVoidEntryLocation = FVector(0.0, 0.0, -10000.0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bUseFixedWhiteVoidEntryLocation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bStopMovementOnTransition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bResetMotionTrajectoryOnTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float WhiteVoidMotionMatchingSuppressDuration = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bPreserveCameraLagStateOnTransition = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bForceCameraCutOnTransition = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bAutoSpawnWhiteVoidSet = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<AGP_WhiteVoidSetActor> WhiteVoidSetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bDebugWhiteVoidTransition = false;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsInWhiteVoid, Category = "White Void", meta = (AllowPrivateAccess = "true"))
	bool bIsInWhiteVoid = false;

	bool bHasStoredWhiteVoidOrigin = false;
	FVector StoredWhiteVoidOriginLocation = FVector::ZeroVector;
	FRotator StoredWhiteVoidOriginRotation = FRotator::ZeroRotator;
	FRotator StoredWhiteVoidControlRotation = FRotator::ZeroRotator;
	bool bStoredCameraLagEnabled = false;
	bool bStoredCameraRotationLagEnabled = false;

	FTimerHandle RestoreLagTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PrimaryAttackAutoFacingTarget;

	float PrimaryAttackAutoFacingEndTime = 0.0f;

	UFUNCTION(Server, Reliable)
	void ServerSetWhiteVoid(bool bNewInWhiteVoid);

	UFUNCTION()
	void OnRep_IsInWhiteVoid();

	void SetWhiteVoidState(bool bNewInWhiteVoid);
	void PerformWhiteVoidTransition(bool bNewInWhiteVoid);
	void CacheWhiteVoidTransitionState(bool bCacheCameraLag = true);
	void RestoreWhiteVoidCameraLag();
	FVector ResolveWhiteVoidTargetLocation(bool bNewInWhiteVoid) const;
	void EnsureWhiteVoidSetExists();
	void ResetMotionTrajectoryAfterWhiteVoidTransition();
	void SuppressMotionMatchingForWhiteVoidTransition() const;
	AActor* FindBestPrimaryAttackTarget(float SearchRadius, float ForwardOffset) const;
	void UpdatePrimaryAttackAutoFacing(float DeltaSeconds);

	// =========================================================================
	// 15. 액션 루트 모션, 관성 & 몽타주 트래킹 (Action Motion Tracking)
	// =========================================================================
	struct FGPActionMotionSample
	{
		FVector Delta = FVector::ZeroVector;
		float DeltaSeconds = 0.0f;
		float TimeSeconds = 0.0f;
	};

	void UpdateActionMotionTracking(float DeltaSeconds);
	void UpdateActionCarryVelocity(float DeltaSeconds);
	void UpdatePrimaryAttackMovementAssist(float DeltaSeconds);
	bool ShouldApplyActionInertiaDirectMovement() const;
	void FlushActionMotionTracking();
	FVector GetCurrentActionInertiaVelocity() const;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveUEFNSourceFallbackMontage;

	bool bApplyUEFNSourceFallbackRootMotion = false;
	bool bActionRootMotionInputCancelEnabled = false;
	bool bBlendActionLowerBodyToMotionMatching = false;
	float ActionLowerBodyMotionMatchBlendTargetAlpha = 1.0f;
	bool bActionRootMotionCancelledByMovementInput = false;
	FVector LastActionRootMotionCancelMovementDirection = FVector::ZeroVector;
	float LastActionRootMotionCancelMovementScale = 0.0f;
	float LastActionRootMotionCancelMovementInputTime = 0.0f;
	FVector LastUEFNSourceRootMotionVelocity = FVector::ZeroVector;
	FVector LastNonZeroUEFNSourceRootMotionVelocity = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation|Runtime Retarget Fallback", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float FallbackRootMotionDistanceCorrection = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Root Motion", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HeldMovementInputCancelGraceTime = 0.50f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true"))
	bool bApplyActionInertiaOnMontageComplete = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true", ClampMin = "0.02"))
	float ActionInertiaSampleWindow = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ActionInertiaMinSampleTime = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ActionInertiaMinSpeed = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float ActionInertiaMaxSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true"))
	bool bCarryEntryVelocityDuringActionRootMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat|Action Inertia", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PostActionAnimVelocityHoldTime = 0.35f;

	bool bPrimaryAttackMovementAssistEnabled = false;
	float PrimaryAttackMovementAssistSpeedRatio = 0.35f;
	bool bPrimaryAttackInProgress = false;
	float LastPrimaryAttackMovementLogTime = -1000.0f;

	bool bTrackActionMotion = false;
	FVector LastActionMotionSampleLocation = FVector::ZeroVector;
	float LastActionMotionSampleTime = 0.0f;
	FVector ActionMotionEntryVelocity = FVector::ZeroVector;
	FVector ActionMotionCarryVelocity = FVector::ZeroVector;
	FVector LastActionCarryActualDelta = FVector::ZeroVector;
	FVector CurrentActionMotionVelocity = FVector::ZeroVector;
	FVector LastNonZeroActionMotionVelocity = FVector::ZeroVector;
	FVector HeldPostActionAnimVelocity = FVector::ZeroVector;
	float HeldPostActionAnimVelocityUntilTime = 0.0f;
	TArray<FGPActionMotionSample> ActionMotionSamples;

	// =========================================================================
	// 16. 가속도 제어 런타임 변수 (Movement Acceleration Runtime States)
	// =========================================================================
	bool bIsStartAccelerationClamped = false;
	float StartAccelerationClampElapsed = 0.0f;
	float NormalMaxAcceleration = 2048.0f;
	FVector LastMoveInputDirection = FVector::ZeroVector;
	float MoveInputReversalGraceTimeRemaining = 0.0f;
	float LastFootstepNoiseTimeSeconds = -1000000.0f;

	// =========================================================================
	// 17. 능력 및 스킬 제어 유틸 함수 (Ability Slot Control Helpers)
	// =========================================================================
	bool GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, UObject* SourceObject = nullptr);
	void ClearAbilitySlot(FGameplayTag SlotTag);
};
