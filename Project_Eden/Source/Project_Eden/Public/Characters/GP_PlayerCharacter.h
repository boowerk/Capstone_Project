#pragma once

#include "CoreMinimal.h"
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

UCLASS()
class PROJECT_EDEN_API AGP_PlayerCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_PlayerCharacter();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void Landed(const FHitResult& Hit) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void AddMovementInput(FVector WorldDirection, float ScaleValue, bool bForce = false) override;
	
	void ToggleSprinting(); 
	void StartSprinting();
	void StopSprinting();
	bool IsSprinting() const; 
	
	bool TryPerformDash();
	bool IsDashing() const;
	bool IsLockOn() const { return bIsLockOn; }

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void ToggleWhiteVoid();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void EnterWhiteVoid();

	UFUNCTION(BlueprintCallable, Category = "White Void")
	void ExitWhiteVoid();

	UFUNCTION(BlueprintPure, Category = "White Void")
	bool IsInWhiteVoid() const { return bIsInWhiteVoid; }

	float GetMovementSpeedScaleRatio() const;
	float GetScaledNormalWalkSpeed() const;
	float GetScaledSprintSpeed() const;
	float ResolveDirectionalMoveSpeed(const FVector2D& MoveInput, bool bSprinting) const;
	const FGPDirectionalMovementSpeedProfile& GetActiveMovementSpeedProfile() const;

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetGASMovementSpeedMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetGASMovementSpeedScaleRatioMultiplier(float NewMultiplier);

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void SetMovementSpeedProfileOverride(const FGPDirectionalMovementSpeedProfile& NewProfile);

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	void ClearMovementSpeedProfileOverride();

	UPDA_CharacterAnimationSet* GetAnimationSet() const { return AnimationSet; }
	UAnimInstance* GetUEFNSourceAnimInstance() const;
	float PlayUEFNSourceFallbackMontage(UAnimMontage* Montage, float PlayRate = 1.0f);
	bool IsPlayingUEFNSourceFallbackMontage() const;
	void StopUEFNSourceFallbackMontage(float BlendOutTime = 0.2f);
	FVector GetLastUEFNSourceRootMotionVelocity() const { return LastUEFNSourceRootMotionVelocity; }
	
	/** [데이터 에셋 기반] 런타임 스킬 교체 함수 (bIgnoreRestrictions로 로그라이크식 예외 지원) */
	UFUNCTION(BlueprintCallable, Category = "GAS|Combat")
	void EquipSkill(UGP_SkillData* NewSkillData, FGameplayTag SlotTag, bool bIgnoreRestrictions = false);

	/** 구형 호환용 (클래스 직접 교체) */
	UFUNCTION(BlueprintCallable, Category = "GAS|Combat", meta = (DeprecatedFunction, DeprecationMessage = "Use DataAsset version instead"))
	void EquipSkillByClass(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> NewAbilityClass);

	UPROPERTY(BlueprintAssignable, Category = "GAS|Events")
	FOnSkillEquipped OnSkillEquipped;


private:
	bool GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, UObject* SourceObject = nullptr);
	void ClearAbilitySlot(FGameplayTag SlotTag);

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Animation|Retarget", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> UEFNSourceMesh;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveUEFNSourceFallbackMontage;

	bool bApplyUEFNSourceFallbackRootMotion = false;
	FVector LastUEFNSourceRootMotionVelocity = FVector::ZeroVector;

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

public:
	virtual void UpdateAnimationSet() override;

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

	
	// GAS 태그 이벤트 콜백
	void OnSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void OnFixedTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	void ApplyMovementSpeedFromAnimationSet();
	void ApplyRetargetVisualScaleFromAnimationSet();
	void RefreshCurrentMaxWalkSpeed();
	void PushMovementSpeedScaleRatioToAnimInstances();
	
	// 가속도 가변 제어 튜닝 설정값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampMaxAcceleration = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampMaxDuration = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	float StartClampReleaseSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Movement|Tuning")
	bool bDebugStartAccelerationClamp = false;

private:
	// 런타임 제어 상태 변수
	bool bIsStartAccelerationClamped = false;
	float StartAccelerationClampElapsed = 0.0f;
	float NormalMaxAcceleration = 2048.0f;
	FVector LastMoveInputDirection = FVector::ZeroVector;
	float MoveInputReversalGraceTimeRemaining = 0.0f;

protected:
	void UpdateConditionalMaxAcceleration(float DeltaSeconds);
	bool ShouldStartAccelerationClamp() const;
	bool ShouldReleaseAccelerationClamp() const;
	void RestoreNormalMaxAcceleration();

};
