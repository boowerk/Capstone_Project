#pragma once

#include "CoreMinimal.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "GP_ShrineRuinsRegionEventActor.generated.h"

class AGP_PlayerCharacter;

/** Region event that grants the regular augment-selection UI when a player reaches shrine ruins. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_ShrineRuinsRegionEventActor : public AGP_RegionEventActor
{
	GENERATED_BODY()

public:
	AGP_ShrineRuinsRegionEventActor();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Shrine")
	bool bCompleteAfterFirstClaim = true;

private:
	UPROPERTY(Transient)
	TSet<TObjectPtr<AGP_PlayerCharacter>> RewardedPlayers;

	UFUNCTION()
	void HandleShrineOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
