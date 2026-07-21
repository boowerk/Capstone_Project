#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_ShrineRuinsRegionEventActor.generated.h"

class AGP_PlayerCharacter;
class AGP_PlayerController;

/** Region event that grants the regular augment-selection UI when a player reaches shrine ruins. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_ShrineRuinsRegionEventActor : public AGP_RegionEventActor
{
	GENERATED_BODY()

public:
	AGP_ShrineRuinsRegionEventActor();

	virtual void ActivateRegionEvent() override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Region Event|Shrine")
	void ConfigureGuidedPartyReward(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Region Event|Shrine")
	bool IsGuidedPartyRewardEnabled() const { return bGuidedPartyReward; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Shrine")
	int32 GetRewardedControllerCount() const { return RewardedControllers.Num(); }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Shrine")
	bool bCompleteAfterFirstClaim = true;

private:
	UPROPERTY(Transient)
	TSet<TObjectPtr<AGP_PlayerController>> RewardedControllers;

	bool bGuidedPartyReward = false;

	UFUNCTION()
	void HandleShrineOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RewardOverlappingPlayers();
	void RewardConnectedPartyPlayers();
	void RewardPartyControllers(const TArray<AGP_PlayerController*>& PartyControllers);
	void TryRewardPlayer(AGP_PlayerCharacter* PlayerCharacter);
	bool TryRewardController(AGP_PlayerController* PlayerController);

#if WITH_DEV_AUTOMATION_TESTS
	friend class FGPRegionEventGuidedDirectorControlTest;
#endif
};
