#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "TimerManager.h"
#include "GP_CrystalSeraphPatternAbility.generated.h"

class AGP_CrystalSeraphBossCharacter;
struct FGameplayEventData;

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphPatternAbility : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphPatternAbility();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor);
	virtual bool UsesSharedPatternCadence() const { return true; }
	virtual bool UsesBossTelegraphVFX() const { return true; }
	AActor* ResolvePatternTarget(const FGameplayEventData* TriggerEventData) const;

private:
	void ExecutePendingPattern();

	FTimerHandle TelegraphTimerHandle;
	TWeakObjectPtr<AActor> PendingPatternTarget;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphShardAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphShardAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphLaserAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphLaserAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphPrismAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphPrismAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphAreaAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphAreaAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphGroggyAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphGroggyAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
	// Groggy is a state reaction and must interrupt immediately instead of waiting for the attack cadence.
	virtual bool UsesSharedPatternCadence() const override { return false; }
	virtual bool UsesBossTelegraphVFX() const override { return false; }
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphTeleportAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphTeleportAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, AActor* PatternTargetActor) override;
	// Teleport is positioning utility rather than an attack pattern.
	virtual bool UsesBossTelegraphVFX() const override { return false; }
};
