#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "TimerManager.h"
#include "GP_BossGroundHandsAttack.generated.h"

class AGP_BossGroundHandActor;
struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_BossGroundHandsAttack : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_BossGroundHandsAttack();

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

private:
	AActor* ResolvePatternTarget(AActor* AvatarActor) const;
	void SpawnScheduledHand(int32 WaveIndex, int32 HandIndex);
	void FinishPattern();

private:
	// A native default keeps BP_Boss_Sans functional before a polished hand Blueprint exists.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands")
	TSubclassOf<AGP_BossGroundHandActor> GroundHandActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "1", ClampMax = "5"))
	int32 WaveCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "1", ClampMax = "6"))
	int32 HandsPerWave = 3;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 0.75f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.0", Units = "s"))
	float HandSpawnInterval = 0.22f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.1", Units = "s"))
	float WaveInterval = 1.55f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.0", Units = "cm"))
	float HandSpacing = 185.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.0", Units = "s"))
	float PatternCooldown = 8.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Ground Hands", meta = (ClampMin = "0.0", Units = "s"))
	float PatternFinishPadding = 1.05f;

	TWeakObjectPtr<AActor> PatternTargetActor;
	TArray<FTimerHandle> SpawnTimerHandles;
	FTimerHandle PatternFinishTimerHandle;
	TArray<TWeakObjectPtr<AGP_BossGroundHandActor>> SpawnedHands;
	float LastPatternStartTime = -BIG_NUMBER;
};
