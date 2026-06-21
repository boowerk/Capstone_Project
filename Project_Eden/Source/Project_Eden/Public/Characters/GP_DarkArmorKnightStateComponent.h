#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP_DarkArmorKnightStateComponent.generated.h"

class AGP_DarkArmorKnightBossCharacter;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class EGPDarkKnightHitDirection : uint8
{
	Front,
	Side,
	Back
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPDarkKnightGuardGaugeChanged, float, GuardGauge, float, MaxGuardGauge);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPDarkKnightStateChanged, bool, bActive);

/** Owns the replicated combat mechanic state; AI and damage execution consume this component instead of duplicating rules. */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_DarkArmorKnightStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_DarkArmorKnightStateComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Dark Knight")
	void InitializeDarkKnightState(AGP_DarkArmorKnightBossCharacter* InBoss);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	bool StartGuardStance(float Duration);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	void EndGuardStance();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	bool StartParryWindow(float Duration);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	void EnterGroggy(float Duration);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	void RecoverFromGroggy();

	/** Called only by the authoritative GAS damage execution; returns the final directional state multiplier. */
	float ResolveIncomingDamageMultiplier(AActor* DamageInstigator, bool bHeavyAttack);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Boss|Dark Knight")
	void SetCombatPhase(int32 NewPhase);

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetGuardGauge() const { return GuardGauge; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetMaxGuardGauge() const { return MaxGuardGauge; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	float GetGuardRatio() const { return MaxGuardGauge > KINDA_SMALL_NUMBER ? GuardGauge / MaxGuardGauge : 0.0f; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool IsGuarding() const { return bIsGuarding; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool IsParryWindowOpen() const { return bParryWindowOpen; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool IsGuardBroken() const { return bGuardBroken; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	bool IsGroggy() const { return bIsGroggy; }

	UFUNCTION(BlueprintPure, Category = "Boss|Dark Knight")
	FName GetLastHitDirectionName() const;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Dark Knight")
	FGPDarkKnightGuardGaugeChanged OnGuardGaugeChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Dark Knight")
	FGPDarkKnightStateChanged OnGuardingChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Dark Knight")
	FGPDarkKnightStateChanged OnParryWindowChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Dark Knight")
	FGPDarkKnightStateChanged OnGroggyChanged;

private:
	EGPDarkKnightHitDirection ResolveHitDirection(const AActor* DamageInstigator) const;
	void SetGuardGaugeInternal(float NewGauge);
	void SetGuardingInternal(bool bNewGuarding);
	void SetParryWindowInternal(bool bNewOpen);
	void SetGroggyInternal(bool bNewGroggy);
	void ApplyStateTags();
	UAbilitySystemComponent* ResolveOwnerASC() const;

	UFUNCTION()
	void OnRep_GuardGauge();

	UFUNCTION()
	void OnRep_IsGuarding();

	UFUNCTION()
	void OnRep_ParryWindowOpen();

	UFUNCTION()
	void OnRep_IsGroggy();

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float PhaseOneMaxGuardGauge = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float FinalPhaseMaxGuardGauge = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float FrontGuardGaugeDamage = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SideGuardGaugeDamage = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float BackGuardGaugeDamage = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float HeavyGuardGaugeDamage = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float FrontDamageMultiplier = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float SideDamageMultiplier = 0.65f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float BackDamageMultiplier = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Damage", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float GroggyDamageMultiplier = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight|Guard", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float GuardMovementSpeedMultiplier = 0.45f;

	UPROPERTY(ReplicatedUsing = OnRep_GuardGauge, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	float GuardGauge = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_GuardGauge, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	float MaxGuardGauge = 100.0f;

	UPROPERTY(ReplicatedUsing = OnRep_IsGuarding, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	bool bIsGuarding = false;

	UPROPERTY(ReplicatedUsing = OnRep_ParryWindowOpen, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	bool bParryWindowOpen = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	bool bGuardBroken = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsGroggy, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	bool bIsGroggy = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	EGPDarkKnightHitDirection LastHitDirection = EGPDarkKnightHitDirection::Front;

	UPROPERTY(Transient)
	TObjectPtr<AGP_DarkArmorKnightBossCharacter> BossOwner;

	FTimerHandle GuardTimerHandle;
	FTimerHandle ParryTimerHandle;
	FTimerHandle GroggyTimerHandle;
	bool bAppliedGuardTag = false;
	bool bAppliedParryTag = false;
	bool bAppliedBrokenTag = false;
	bool bAppliedGroggyTag = false;
	float UnguardedMaxWalkSpeed = 0.0f;
};
