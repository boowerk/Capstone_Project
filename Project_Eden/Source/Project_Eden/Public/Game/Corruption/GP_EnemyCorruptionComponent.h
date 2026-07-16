#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "GP_EnemyCorruptionComponent.generated.h"

class UAbilitySystemComponent;
class UGP_WorldCorruptionComponent;

/**
 * Enemy-side adapter between replicated world corruption and GAS attributes.
 * It owns only the corruption contribution, so normal initialization and other buffs remain independent.
 */
UCLASS(ClassGroup = (World), meta = (BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_EnemyCorruptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGP_EnemyCorruptionComponent();

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// EnemyCharacter calls this after its startup attribute effect has completed.
	void InitializeFromOwner();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption|Enemy")
	void SetCorruptionRegionId(int32 InRegionId);

	UFUNCTION(BlueprintPure, Category = "World Corruption|Enemy")
	int32 GetCorruptionRegionId() const { return CorruptionRegionId; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Enemy")
	float GetAppliedCorruptionNormalized() const { return AppliedCorruptionNormalized; }

	// Called by the shared enemy death path; regular enemies no-op here.
	void HandleOwnerDeath(bool bWasBoss);

protected:
	// At maximum corruption, enemies deal 50% more damage through the existing GAS damage calculation.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Enemy", meta = (ClampMin = "0.0"))
	float MaximumDamageIncreaseRate = 0.5f;

	// Flat armor is used because multiplicative armor would not strengthen enemies whose base armor is zero.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Enemy", meta = (ClampMin = "0.0"))
	float MaximumArmorBonus = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Boss", meta = (ClampMin = "0.0"))
	float BossDefeatCorruptionReduction = 25.0f;

private:
	int32 CorruptionRegionId = INDEX_NONE;
	float AppliedCorruptionNormalized = 0.0f;
	bool bOwnerDead = false;
	bool bBossReductionApplied = false;
	FActiveGameplayEffectHandle ActiveCorruptionEffectHandle;
	TWeakObjectPtr<UGP_WorldCorruptionComponent> BoundWorldCorruption;

	UFUNCTION()
	void HandleWorldCorruptionChanged(float Corruption, float NormalizedCorruption);

	UFUNCTION()
	void HandleRegionCorruptionChanged(int32 RegionId, float Corruption, float NormalizedCorruption);

	void BindToWorldCorruption();
	void UnbindFromWorldCorruption();
	void RefreshCorruptionEffect();
	int32 ResolveEffectiveRegionId() const;
	UGP_WorldCorruptionComponent* ResolveWorldCorruption() const;
	UAbilitySystemComponent* ResolveOwnerASC() const;
};
