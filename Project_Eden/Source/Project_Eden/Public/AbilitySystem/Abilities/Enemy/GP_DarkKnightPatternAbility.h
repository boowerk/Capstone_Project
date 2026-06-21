#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_DarkKnightPatternAbility.generated.h"

class AGP_DarkArmorKnightBossCharacter;
struct FGameplayEventData;

UCLASS(Abstract, Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightPatternAbility : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_DarkKnightPatternAbility();

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
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor);
	virtual bool UsesSharedPatternCadence() const { return true; }
	void SetPatternTag(const FGameplayTag& PatternTag);
	FGameplayTag GetPatternTag() const;
	AActor* ResolvePatternTarget(const FGameplayEventData* TriggerEventData) const;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightBasicAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightBasicAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightHeavyAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightHeavyAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightSweepAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightSweepAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightGuardAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightGuardAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightChargeAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightChargeAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightDarkWaveAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightDarkWaveAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightGroundCrackAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()
public:
	UGP_DarkKnightGroundCrackAbility();
protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightCounterAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()

public:
	UGP_DarkKnightCounterAbility();

protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
	virtual bool UsesSharedPatternCadence() const override { return false; }
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_DarkKnightGroggyAbility : public UGP_DarkKnightPatternAbility
{
	GENERATED_BODY()

public:
	UGP_DarkKnightGroggyAbility();

protected:
	virtual bool ExecuteDarkKnightPattern(AGP_DarkArmorKnightBossCharacter* Boss, AActor* TargetActor) override;
	virtual bool UsesSharedPatternCadence() const override { return false; }
};
