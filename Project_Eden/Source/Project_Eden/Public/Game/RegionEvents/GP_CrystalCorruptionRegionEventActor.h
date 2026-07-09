#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_CrystalCorruptionRegionEventActor.generated.h"

class AGP_PlayerCharacter;
class AGP_RegionEventCrystalNode;

/** Region event that slows nearby players until spawned corruption crystals are destroyed. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalCorruptionRegionEventActor : public AGP_RegionEventActor
{
	GENERATED_BODY()

public:
	AGP_CrystalCorruptionRegionEventActor();

	virtual void ActivateRegionEvent() override;
	virtual void CompleteRegionEvent() override;
	virtual void ExpireRegionEvent() override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption")
	TSubclassOf<AGP_RegionEventCrystalNode> CrystalNodeClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption", meta = (ClampMin = "1"))
	int32 CrystalCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption", meta = (ClampMin = "0.0"))
	float CrystalRingRadius = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption", meta = (ClampMin = "0.0"))
	float SlowRadius = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SlowMultiplier = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal Corruption", meta = (ClampMin = "0.05"))
	float SlowRefreshInterval = 0.25f;

private:
	struct FSlowedPlayerRecord
	{
		TWeakObjectPtr<AGP_PlayerCharacter> Player;
		float PreviousMultiplier = 1.0f;
	};

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_RegionEventCrystalNode>> ActiveCrystals;

	TArray<FSlowedPlayerRecord> SlowedPlayers;
	FTimerHandle SlowRefreshTimerHandle;
	bool bCorruptionSlowActive = false;

	void SpawnCrystalNodes();
	void DestroyRemainingCrystals();
	void RefreshPlayerSlow();
	void RestoreAllSlowedPlayers();
	int32 FindSlowedPlayerIndex(const AGP_PlayerCharacter* PlayerCharacter) const;

	UFUNCTION()
	void HandleCrystalDestroyed(AGP_RegionEventCrystalNode* CrystalNode, AActor* DestroyingActor);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSetCorruptionSlowActive(bool bActive);
};
