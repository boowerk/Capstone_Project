#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
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
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData);
	virtual bool UsesSharedPatternCadence() const { return true; }
	AActor* ResolvePatternTarget(const FGameplayEventData* TriggerEventData) const;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphShardAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphShardAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
	// Groggy is a state reaction and must interrupt immediately instead of waiting for the attack cadence.
	virtual bool UsesSharedPatternCadence() const override { return false; }
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphLaserAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphLaserAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphPrismAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphPrismAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphAreaAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphAreaAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphGroggyAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphGroggyAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CrystalSeraphTeleportAbility : public UGP_CrystalSeraphPatternAbility
{
	GENERATED_BODY()

public:
	UGP_CrystalSeraphTeleportAbility();

protected:
	virtual bool ExecuteCrystalSeraphPattern(AGP_CrystalSeraphBossCharacter* CrystalSeraphBoss, const FGameplayEventData* TriggerEventData) override;
};
