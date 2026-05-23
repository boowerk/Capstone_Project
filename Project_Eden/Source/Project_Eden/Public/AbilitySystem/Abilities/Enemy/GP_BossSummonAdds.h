#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "GP_BossSummonAdds.generated.h"

class AGP_EnemyCharacter;
struct FGameplayEventData;

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_BossSummonAdds : public UGP_GameplayAbility
{
	GENERATED_BODY()

public:
	UGP_BossSummonAdds();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// Designers can swap the summoned add type while the default stays on the melee enemy requested for Sans.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Summon")
	TSubclassOf<AGP_EnemyCharacter> SummonedEnemyClass;

	// Spawn count is intentionally small so the boss pattern changes pressure without flooding the map.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Summon", meta = (ClampMin = "1", ClampMax = "8"))
	int32 SummonCount = 2;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Summon", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnRadius = 520.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Summon", meta = (ClampMin = "0.0", Units = "cm"))
	float SpawnForwardOffset = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Boss|Summon", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxAliveSummonedAdds = 4;

private:
	int32 CountAliveSummonedAdds();
	bool TrySpawnSummonedAdd(AActor* AvatarActor, int32 SpawnIndex, int32 SpawnTotal);

	TArray<TWeakObjectPtr<AGP_EnemyCharacter>> SummonedAdds;
};
