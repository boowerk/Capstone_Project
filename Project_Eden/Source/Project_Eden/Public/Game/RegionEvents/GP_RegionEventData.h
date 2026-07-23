#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Game/GP_EnemySpawnVolume.h"
#include "GP_RegionEventData.generated.h"

class UTexture2D;
class AGP_RegionEventActor;

/** Presentation/gameplay category used by the region event director and BP widgets. */
UENUM(BlueprintType)
enum class EGPRegionEventType : uint8
{
	EnemyAmbush UMETA(DisplayName = "Enemy Ambush"),
	// Legacy presentation label used by the fixed Red Rift asset; it no longer drives a world-corruption system.
	CorruptionRift UMETA(DisplayName = "Corruption Rift"),
	ShrineCache UMETA(DisplayName = "Shrine Cache"),
	EnvironmentalHazard UMETA(DisplayName = "Environmental Hazard")
};

/**
 * Designer-authored definition for one deterministic graduation-demo encounter.
 * Runtime actors read this data by stable EventId; no weighted or ambient selection remains.
 */
UCLASS(BlueprintType)
class PROJECT_EDEN_API UGP_RegionEventData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	FName EventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (MultiLine = "true"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	EGPRegionEventType EventType = EGPRegionEventType::EnemyAmbush;

	// Optional per-event runtime class. Leave empty to use the director's default actor class.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	TSubclassOf<AGP_RegionEventActor> EventActorClass;

	// Proximity activation keeps an event discoverable in-world before committing the party to its gameplay.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Discovery")
	bool bActivateWhenPlayerApproaches = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Discovery", meta = (EditCondition = "bActivateWhenPlayerApproaches", ClampMin = "0.0"))
	float ApproachActivationRadius = 700.0f;

	// Negative values wait indefinitely; non-negative values fail an ignored dormant event after this many seconds.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Discovery", meta = (EditCondition = "bActivateWhenPlayerApproaches", ClampMin = "-1.0"))
	float DormantWaitTimeoutSeconds = -1.0f;

	// Negative duration means the event stays alive until explicitly completed.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (ClampMin = "-1.0"))
	float DurationSeconds = 20.0f;

	// Optional map/notification icon for future HUD callouts.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	TObjectPtr<UTexture2D> Icon = nullptr;

	// Optional region-state override while the event is active, e.g. a corrupted biome pulse.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Region State")
	bool bApplyActiveRegionState = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Region State", meta = (EditCondition = "bApplyActiveRegionState"))
	uint8 ActiveRegionState = 2;

	// Optional state written when the event succeeds; leave disabled when the zone clear should own revival.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Region State")
	bool bApplyCompletedRegionState = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Region State", meta = (EditCondition = "bApplyCompletedRegionState"))
	uint8 CompletedRegionState = 0;

	// Enemy waves make an event immediately visible in a presentation slice without requiring extra Blueprint logic.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Enemy")
	TArray<FGP_EnemySpawnEntry> EnemySpawns;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Enemy", meta = (ClampMin = "0.0"))
	float EnemySpawnScatterRadius = 450.0f;
};
