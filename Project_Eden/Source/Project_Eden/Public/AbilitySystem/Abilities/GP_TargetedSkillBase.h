#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_SkillBase.h"
#include "Engine/EngineTypes.h"
#include "GP_TargetedSkillBase.generated.h"

class UAbilityTask_WaitGameplayEvent;

UENUM(BlueprintType)
enum class EGP_SkillSelectionMode : uint8
{
	Instant,
	Projectile,
	Ray,
	TargetActor,
	GroundPosition
};

UENUM(BlueprintType)
enum class EGP_SkillConfirmType : uint8
{
	Primary,
	Secondary
};

USTRUCT(BlueprintType)
struct FGP_SkillTargetData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	EGP_SkillSelectionMode SelectionMode = EGP_SkillSelectionMode::Instant;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	EGP_SkillConfirmType ConfirmType = EGP_SkillConfirmType::Primary;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	FVector Origin = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	FVector AimDirection = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	FVector TargetLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Skill|Target")
	bool bBlockingHit = false;
};

/**
 * Base class for skills that enter a selectable/aiming state before they commit.
 * The selection phase can move with the player, preview a target, and be confirmed or cancelled by GAS events.
 */
UCLASS(Abstract)
class PROJECT_EDEN_API UGP_TargetedSkillBase : public UGP_SkillBase
{
	GENERATED_BODY()

public:
	UGP_TargetedSkillBase();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UFUNCTION(BlueprintNativeEvent, Category = "GAS|Skill|Target")
	void ExecuteConfirmedSkill(const FGP_SkillTargetData& TargetData);
	virtual void ExecuteConfirmedSkill_Implementation(const FGP_SkillTargetData& TargetData);

	UFUNCTION(BlueprintPure, Category = "GAS|Skill|Target")
	FGP_SkillTargetData GetCurrentTargetData(EGP_SkillConfirmType ConfirmType = EGP_SkillConfirmType::Primary) const;

	virtual float GetPreviewActorRadius() const;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	EGP_SkillSelectionMode SelectionMode = EGP_SkillSelectionMode::Instant;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	bool bAllowSecondaryConfirm = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	bool bRequireBlockingHit = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection", meta = (ClampMin = "1.0"))
	float MaxTargetRange = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection", meta = (ClampMin = "0.01"))
	float PreviewUpdateInterval = 0.03f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECC_Visibility;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	TSubclassOf<AActor> TargetActorClassFilter;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	bool bUseAimAssistTargetSelection = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection")
	bool bTargetSelectionRequiresCombatEffect = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection", meta = (ClampMin = "1.0"))
	float TargetAimAssistRadius = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection", meta = (ClampMin = "0.0"))
	float GroundTraceHeight = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Selection", meta = (ClampMin = "0.0"))
	float GroundTraceDepth = 2000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Preview")
	TSubclassOf<AActor> PreviewActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Preview")
	bool bDrawSelectionDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GAS|Skill|Lifecycle")
	bool bEndAbilityAfterConfirmedExecute = true;

	bool HasSelectionLineOfSight(const AActor* AvatarActor, const AActor* CandidateActor) const;
	bool IsActorBlockingSkillLineOfSight(const AActor* HitActor) const;

private:
	void BeginSelection();
	void CleanupSelection();
	void RegisterSelectionEvents();
	void AddSelectionLooseTags();
	void RemoveSelectionLooseTags();
	void UpdatePreview();
	void ConfirmSelection(EGP_SkillConfirmType ConfirmType);
	void ConfirmSelectionFromPayload(const FGameplayEventData& Payload, EGP_SkillConfirmType ConfirmType);
	bool TryCommitAndExecute(const FGP_SkillTargetData& TargetData);
	bool ResolveGroundLocation(const AActor* AvatarActor, const FVector& CandidateLocation, FVector& OutGroundLocation) const;
	FGP_SkillTargetData ValidateReceivedTargetData(const FGP_SkillTargetData& TargetData) const;
	AActor* FindBestTargetActor(const AActor* AvatarActor, const FVector& TraceStart, const FVector& AimDirection) const;

	UFUNCTION()
	void OnPrimaryConfirm(FGameplayEventData Payload);

	UFUNCTION()
	void OnSecondaryConfirm(FGameplayEventData Payload);

	UFUNCTION()
	void OnCancelSelection(FGameplayEventData Payload);

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> PrimaryConfirmTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> SecondaryConfirmTask;

	UPROPERTY(Transient)
	TObjectPtr<UAbilityTask_WaitGameplayEvent> CancelTask;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PreviewActor;

	FVector PreviewActorBaseScale = FVector::OneVector;
	float PreviewActorBaseRadius = 0.0f;
	FTimerHandle PreviewTimerHandle;
	FGameplayTagContainer AddedLooseTags;
	bool bSelectionActive = false;
};
