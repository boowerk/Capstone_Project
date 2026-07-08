#pragma once

#include "CoreMinimal.h"
#include "Actors/GP_MatadorDecoyPressureComponent.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_MatadorDecoyAnimInstance.generated.h"

class AGP_MatadorBossDecoyActor;
class UAnimMontage;

UENUM(BlueprintType)
enum class EGPMatadorDecoyPresentationState : uint8
{
	Idle,
	RapierAim,
	RapierLocked,
	RapierThrust,
	CapePrepare,
	CapeLocked,
	CapeGust
};

UCLASS(Blueprintable, BlueprintType)
class PROJECT_EDEN_API UGP_MatadorDecoyAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	UGP_MatadorDecoyAnimInstance();

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	AGP_MatadorBossDecoyActor* GetMatadorDecoyOwner() const { return DecoyOwner.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	EGPMatadorDecoyPressureState GetPressureState() const { return PressureState; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	bool IsWalkingPressure() const { return bIsWalkingPressure; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	EGPMatadorDecoyPresentationState GetPresentationState() const { return PresentationState; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	float GetPresentationStateTime() const { return PresentationStateTime; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy")
	float GetPresentationStateDuration() const { return PresentationStateDuration; }

	void NotifyRapierAimStarted(float InAimDuration);
	void NotifyRapierDirectionLocked(FVector LockedDirection, float InCommitDelay);
	void NotifyRapierThrust(FVector LockedDirection);
	void NotifyCapePrepareStarted(float InPrepareDuration);
	void NotifyCapeDirectionLocked(FVector LockedDirection);
	void NotifyCapeGustBurst(FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount);

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	TObjectPtr<AGP_MatadorBossDecoyActor> DecoyOwner;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	TObjectPtr<UGP_MatadorDecoyPressureComponent> PressureComponent;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	EGPMatadorDecoyPressureState PressureState = EGPMatadorDecoyPressureState::Inactive;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	TObjectPtr<AActor> CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	bool bIsWalkingPressure = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	bool bTeleportRequested = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	bool bPostTeleportAttackLocked = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy")
	int32 StepThrustIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	EGPMatadorDecoyPresentationState PresentationState = EGPMatadorDecoyPresentationState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	FVector PresentationDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	float PresentationStateTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	float PresentationStateDuration = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bRapierAimActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bRapierLockedActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bRapierThrustActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bCapePrepareActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bCapeLockedActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	bool bCapeGustActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	int32 CapeBurstIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation")
	int32 CapeBurstCount = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation|Montage")
	TObjectPtr<UAnimMontage> RapierThrustMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation|Montage")
	TObjectPtr<UAnimMontage> CapeGustMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation|Montage", meta = (ClampMin = "0.01"))
	float RapierThrustMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation|Montage", meta = (ClampMin = "0.01"))
	float CapeGustMontagePlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Decoy|Presentation|Montage")
	bool bPlayCapeGustMontageOnEveryBurst = false;

private:
	void CacheDecoyOwner();
	void SetPresentationState(EGPMatadorDecoyPresentationState NewState, FVector Direction, float Duration);
	void RefreshPresentationFlags();
	void PlayPresentationMontage(UAnimMontage* Montage, float PlayRate);

	FVector PreviousOwnerLocation = FVector::ZeroVector;
	bool bHasPreviousOwnerLocation = false;
};
