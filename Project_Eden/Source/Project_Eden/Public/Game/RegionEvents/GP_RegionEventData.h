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
	CorruptionRift UMETA(DisplayName = "Corruption Rift"),
	ShrineCache UMETA(DisplayName = "Shrine Cache"),
	EnvironmentalHazard UMETA(DisplayName = "Environmental Hazard")
};

/** When the run flow is allowed to roll this event for a zone. */
UENUM(BlueprintType)
enum class EGPRegionEventTrigger : uint8
{
	ZoneStarted UMETA(DisplayName = "Zone Started"),
	ZoneCompleted UMETA(DisplayName = "Zone Completed")
};

/**
 * Designer-authored definition for one possible event on a PCG/region slot.
 * Runtime actors read this data, while Blueprint children can replace art/VFX later.
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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	EGPRegionEventTrigger Trigger = EGPRegionEventTrigger::ZoneStarted;

	// Optional per-event runtime class. Leave empty to use the director's default actor class.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	TSubclassOf<AGP_RegionEventActor> EventActorClass;

	// Weight is kept on the data asset so content designers can tune the pool without touching C++.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (ClampMin = "0.0"))
	float SelectionWeight = 1.0f;

	// Corruption bounds let the director keep an event out of regions where its fiction would not make sense.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Corruption", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumCorruptionNormalized = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Corruption", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaximumCorruptionNormalized = 1.0f;

	// Outcome deltas are raw corruption points, matching UGP_WorldCorruptionComponent's public API.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Corruption", meta = (ClampMin = "0.0"))
	float CompletionCorruptionReduction = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Corruption", meta = (ClampMin = "0.0"))
	float ExpirationCorruptionIncrease = 0.0f;

	// Zero preserves the legacy behavior where this event may be selected again immediately.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Flow", meta = (ClampMin = "0.0"))
	float RegionCooldownSeconds = 0.0f;

	// Blocking is opt-in so existing event assets cannot unexpectedly prevent zone completion.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Flow")
	bool bBlocksZoneCompletion = false;

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
