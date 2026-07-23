#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GP_CrystalSeraphStateComponent.generated.h"

class UAbilitySystemComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPCrystalSeraphWingCoreStageChangedSignature, int32, WingCoreBreakCount, int32, WingCoreBreakTarget);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPCrystalSeraphWingCoreExposedChangedSignature, bool, bIsWingCoreExposed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPCrystalSeraphGroggyChangedSignature, bool, bIsGroggy);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_CrystalSeraphStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphStateComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializeCrystalSeraphState(AActor* InMainBossActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RegisterCrystalPrismActor(AActor* InCrystalPrismActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void BeginWingCoreExposure(float BossMaxHealth, float ExposureDurationOverride = -1.0f);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void EndWingCoreExposure();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool RecordWingCoreDamage(float DamageAmount, float BossMaxHealth);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void SetWingCoreBreakCount(int32 NewCount);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void ResetWingCoreBreakCount();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void EnterGroggy();

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void RecoverFromGroggy();

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	int32 GetWingCoreBreakCount() const { return WingCoreBreakCount; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	int32 GetWingCoreBreakTarget() const { return FMath::Max(1, WingCoreBreakTarget); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool IsGroggy() const { return bIsGroggy; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool IsGuarded() const { return bIsGuarded; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	bool IsWingCoreExposed() const { return bWingCoreExposed; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetWingCoreHealth() const { return WingCoreHealth; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetWingCoreMaxHealth() const { return WingCoreMaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	AActor* GetMainBossActor() const { return MainBossActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	AActor* GetCrystalPrismActor() const { return CrystalPrismActor.Get(); }

	UPROPERTY(BlueprintAssignable, Category = "Boss|Crystal Seraph")
	FGPCrystalSeraphWingCoreStageChangedSignature OnWingCoreStageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Crystal Seraph")
	FGPCrystalSeraphWingCoreExposedChangedSignature OnWingCoreExposedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Boss|Crystal Seraph")
	FGPCrystalSeraphGroggyChangedSignature OnGroggyChanged;

private:
	UFUNCTION()
	void OnRep_WingCoreBreakCount();

	UFUNCTION()
	void OnRep_IsGroggy();

	UFUNCTION()
	void OnRep_IsGuarded();

	UFUNCTION()
	void OnRep_WingCoreExposed();

	void SetGroggyInternal(bool bNewGroggy);
	void SetGuardedInternal(bool bNewGuarded);
	void SetWingCoreExposedInternal(bool bNewExposed);
	void ResetWingCoreHealth(float BossMaxHealth);
	void ApplyStateTags();
	UAbilitySystemComponent* ResolveOwnerASC() const;

private:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 WingCoreBreakTarget = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float WingCoreHealthFraction = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float DefaultWingCoreExposureDuration = 6.0f;

	UPROPERTY(ReplicatedUsing = OnRep_WingCoreBreakCount, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	int32 WingCoreBreakCount = 0;

	UPROPERTY(ReplicatedUsing = OnRep_IsGroggy, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	bool bIsGroggy = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsGuarded, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	bool bIsGuarded = true;

	UPROPERTY(ReplicatedUsing = OnRep_WingCoreExposed, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	bool bWingCoreExposed = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	float WingCoreHealth = 0.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	float WingCoreMaxHealth = 1.0f;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> MainBossActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CrystalPrismActor;

	FTimerHandle WingCoreExposureTimerHandle;
	bool bAppliedCrystalGuardedTag = false;
	bool bAppliedWingCoreExposedTag = false;
	bool bAppliedGroggyTag = false;
};
