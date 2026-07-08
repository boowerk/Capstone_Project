#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP_MatadorDecoyPressureComponent.generated.h"

class UGP_MatadorBossStateComponent;

UENUM(BlueprintType)
enum class EGPMatadorDecoyPressureState : uint8
{
	Inactive,
	SelectTarget,
	WalkApproach,
	TeleportReserved,
	PreTeleportGrace,
	PostTeleportGrace,
	PatternActionLocked
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPMatadorDecoyTargetChangedSignature, AActor*, NewTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPMatadorDecoyTeleportSignature, AActor*, NewTarget, FVector, TeleportLocation);

/**
 * Server-authoritative pressure controller for Matador decoys.
 *
 * This does not own the authored attack montage yet. It provides the reusable target,
 * walking approach, teleport reservation, and post-teleport lock layer that the step-thrust
 * ability/montage can drive later.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_MatadorDecoyPressureComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_MatadorDecoyPressureComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void InitializePressure(AActor* InMainBossActor, UGP_MatadorBossStateComponent* InStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void StartPressure();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void StopPressure();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void RequestTeleport(bool bAdvanceComboOnTeleport = true);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void NotifyPatternActionStarted();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void NotifyPatternActionFinished();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Decoy Pressure")
	void AdvanceStepThrustIndex();

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	EGPMatadorDecoyPressureState GetPressureState() const { return PressureState; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	bool IsTeleportRequested() const { return bTeleportRequested; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	bool IsPostTeleportAttackLocked() const { return bPostTeleportAttackLocked; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	bool IsPressureActive() const { return bPressureActive; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador|Decoy Pressure")
	int32 GetStepThrustIndex() const { return StepThrustIndex; }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Matador|Decoy Pressure")
	FGPMatadorDecoyTargetChangedSignature OnTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Matador|Decoy Pressure")
	FGPMatadorDecoyTeleportSignature OnTeleportExecuted;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure")
	bool bAutoStartPressure = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm/s"))
	float WalkSpeed = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm"))
	float MeleeEnterRange = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm"))
	float MeleeExitRange = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "s"))
	float TargetOutOfRangeTime = 2.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "s"))
	float PreTeleportGraceTime = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "s"))
	float PostTeleportGraceTime = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm"))
	float TeleportMinDistance = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm"))
	float TeleportPreferredDistance = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "0.0", Units = "cm"))
	float TeleportMaxDistance = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure", meta = (ClampMin = "1"))
	int32 MaxNormalStepThrustCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Decoy Pressure")
	bool bAvoidImmediateSameTarget = true;

private:
	bool HasAuthority() const;
	bool IsPressureAllowed() const;
	bool IsTargetValid(AActor* Candidate) const;
	AActor* SelectRandomTarget();
	void SetCurrentTarget(AActor* NewTarget);
	void UpdateWalkApproach(float DeltaTime);
	void UpdateDistanceAndTeleportRequest(float DeltaTime);
	void UpdateTeleportFlow();
	void ExecuteReservedTeleport();
	bool ResolveTeleportLocation(AActor* TargetActor, FVector& OutLocation) const;
	float GetOwnerCapsuleHalfHeight() const;
	FRotator ResolveFacingRotation(const FVector& FromLocation, const FVector& ToLocation) const;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> MainBossActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CurrentTarget;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> PreviousTarget;

	UPROPERTY(Transient)
	EGPMatadorDecoyPressureState PressureState = EGPMatadorDecoyPressureState::Inactive;

	bool bPressureActive = false;
	bool bPatternActionLocked = false;
	bool bTeleportRequested = false;
	bool bAdvanceComboWhenTeleporting = true;
	bool bPostTeleportAttackLocked = false;
	float OutOfRangeElapsed = 0.0f;
	float PreTeleportReadyTime = 0.0f;
	float PostTeleportUnlockTime = 0.0f;
	int32 StepThrustIndex = 0;
};
