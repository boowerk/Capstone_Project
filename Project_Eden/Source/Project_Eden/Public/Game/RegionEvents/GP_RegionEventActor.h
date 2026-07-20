#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventActor.generated.h"

class AGP_EnemyCharacter;
class UDecalComponent;
class UGP_RegionEventData;
class UMaterialInterface;
class UPointLightComponent;
class USphereComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGPRegionEventRuntimeState : uint8
{
	Dormant UMETA(DisplayName = "Dormant"),
	Active UMETA(DisplayName = "Active"),
	Completed UMETA(DisplayName = "Completed"),
	Expired UMETA(DisplayName = "Expired")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnRegionEventEnemySpawned, class AGP_RegionEventActor*, EventActor, AGP_EnemyCharacter*, Enemy);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnRegionEventStateChanged, class AGP_RegionEventActor*, EventActor);

/**
 * Replicated runtime instance for one selected region event.
 * It owns presentation hooks and optional event enemy spawning; the director decides where/when it exists.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void InitializeRegionEvent(int32 InRegionId, UGP_RegionEventData* InEventData);

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	virtual void ActivateRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	virtual void CompleteRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	virtual void ExpireRegionEvent();

	UFUNCTION(BlueprintPure, Category = "Region Event")
	int32 GetRegionId() const { return RegionId; }

	UFUNCTION(BlueprintPure, Category = "Region Event")
	UGP_RegionEventData* GetEventData() const { return EventData; }

	UFUNCTION(BlueprintPure, Category = "Region Event")
	EGPRegionEventRuntimeState GetRuntimeState() const { return RuntimeState; }

	// Runtime spawners can override DataAsset discovery settings without reaching into actor internals.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Region Event|Discovery")
	void ConfigureApproachActivation(bool bWaitForPlayerApproach, float InActivationRadius, float InDormantWaitTimeoutSeconds);

	// Director-facing shorthand always enables proximity discovery with the supplied runtime tuning.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Region Event|Discovery")
	void ConfigureExplorationActivation(float InActivationRadius, float InDormantTimeoutSeconds);

	// Guided objectives wait for a party quorum; the effective requirement clamps to currently possessed players.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Region Event|Discovery")
	void ConfigureGuidedExplorationActivation(
		float InActivationRadius,
		float InDormantTimeoutSeconds,
		int32 InRequiredApproachPlayers);

	UFUNCTION(BlueprintPure, Category = "Region Event|Discovery")
	bool IsWaitingForPlayerApproach() const { return bWaitingForPlayerApproach; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Discovery")
	int32 GetRequiredApproachPlayerCount() const { return RequiredApproachPlayerCount; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Enemy")
	int32 GetAliveSpawnedEnemyCount() const;

	UFUNCTION(BlueprintPure, Category = "Region Event|Enemy")
	bool ShouldRetireSpawnedEnemiesOnEnd() const { return bRetireSpawnedEnemiesOnEnd; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Flow")
	bool IsBlockingZoneCompletion() const;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnemySpawned OnRegionEventEnemySpawned;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventStateChanged OnRegionEventStateChanged;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event")
	TObjectPtr<USphereComponent> ActivationTrigger;

	// Native fallback presentation keeps a C++-only event visible; Blueprint children may replace every asset/tuning value.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	TObjectPtr<UDecalComponent> EventMarkerDecal;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	TObjectPtr<UPointLightComponent> EventMarkerLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	TObjectPtr<UTextRenderComponent> EventMarkerText;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	TObjectPtr<UMaterialInterface> MarkerDecalMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	FVector MarkerDecalSize = FVector(120.0f, 500.0f, 500.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	bool bShowWorldMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	FLinearColor DormantMarkerColor = FLinearColor(0.15f, 0.65f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	FLinearColor ActiveMarkerColor = FLinearColor(1.0f, 0.2f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	FLinearColor CompletedMarkerColor = FLinearColor(0.15f, 1.0f, 0.25f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation")
	FLinearColor ExpiredMarkerColor = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Presentation", meta = (ClampMin = "0.0"))
	float MarkerLightIntensity = 2500.0f;

	// Enables designer-authored exploration events that wait until the party reaches the spawned marker.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	bool bActivateOnPlayerOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (ClampMin = "0.0"))
	float ActivationRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Spawn", meta = (ClampMin = "0.0"))
	float SpawnZOffset = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Spawn")
	FVector NavProjectionExtent = FVector(650.0f, 650.0f, 1200.0f);

	// Timed objectives must retire their remaining wave enemies so repeated open-world events cannot leak AI actors.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Enemy")
	bool bRetireSpawnedEnemiesOnEnd = true;

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventExpired();

	void SetRuntimeState(EGPRegionEventRuntimeState NewState);
	void ApplyRegionState(uint8 NewState) const;
	int32 SpawnConfiguredEnemies();
	FVector ResolveSpawnLocation(float ScatterRadius, bool& bOutProjected) const;

	// Specialized event actors can react when their self-owned encounter enemy count changes.
	virtual void OnTrackedEnemyCountChanged();

private:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	int32 RegionId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_EventData, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_RegionEventData> EventData = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	EGPRegionEventRuntimeState RuntimeState = EGPRegionEventRuntimeState::Dormant;

	FTimerHandle AutoExpireTimerHandle;
	FTimerHandle DormantWaitTimerHandle;

	// Strong references keep event-spawned enemies trackable until their authoritative death callback arrives.
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemyCharacter>> SpawnedEventEnemies;

	bool bHasApproachActivationOverride = false;
	bool bApproachActivationOverride = false;
	bool bApproachActivationRequested = false;
	bool bWaitingForPlayerApproach = false;
	float ApproachActivationRadiusOverride = 700.0f;
	float DormantWaitTimeoutOverride = -1.0f;
	int32 RequiredApproachPlayerCount = 1;

	void DispatchStatePresentation(EGPRegionEventRuntimeState PresentedState);
	void RefreshWorldMarkerPresentation();
	void BeginWaitingForPlayerApproach();
	void StopWaitingForPlayerApproach();
	void TryActivateFromCurrentOverlaps();
	bool HasApproachPlayerQuorum(int32 OverlappingPlayerCount, int32 PossessedPlayerCount) const;
	void HandleDormantWaitTimeout();
	void ApplyCorruptionOutcome(bool bCompletedSuccessfully) const;
	void RetireSpawnedEnemies();
	bool ShouldWaitForPlayerApproach() const;
	float GetConfiguredApproachRadius() const;
	float GetConfiguredDormantWaitTimeout() const;
	FLinearColor GetMarkerColorForState(EGPRegionEventRuntimeState State) const;
	FText GetMarkerTextForState(EGPRegionEventRuntimeState State) const;

	UFUNCTION()
	void OnRep_EventData();

	UFUNCTION()
	void OnRep_RuntimeState(EGPRegionEventRuntimeState PreviousState);

	UFUNCTION()
	void HandleSpawnedEventEnemyDied(AGP_EnemyCharacter* DeadEnemy, AActor* DeathInstigator);

	UFUNCTION()
	void HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

#if WITH_DEV_AUTOMATION_TESTS
	friend class FGPRegionEventGuidedDirectorControlTest;
#endif
};
