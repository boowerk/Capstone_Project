#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventActor.generated.h"

class AGP_EnemyCharacter;
class UGP_RegionEventData;
class USphereComponent;

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
	void ActivateRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void CompleteRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event")
	void ExpireRegionEvent();

	UFUNCTION(BlueprintPure, Category = "Region Event")
	int32 GetRegionId() const { return RegionId; }

	UFUNCTION(BlueprintPure, Category = "Region Event")
	UGP_RegionEventData* GetEventData() const { return EventData; }

	UFUNCTION(BlueprintPure, Category = "Region Event")
	EGPRegionEventRuntimeState GetRuntimeState() const { return RuntimeState; }

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventEnemySpawned OnRegionEventEnemySpawned;

	UPROPERTY(BlueprintAssignable, Category = "Region Event")
	FGPOnRegionEventStateChanged OnRegionEventStateChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event")
	TObjectPtr<USphereComponent> ActivationTrigger;

	// Enables designer-authored exploration events that wait until the party reaches the spawned marker.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event")
	bool bActivateOnPlayerOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event", meta = (ClampMin = "0.0"))
	float ActivationRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Spawn", meta = (ClampMin = "0.0"))
	float SpawnZOffset = 40.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Spawn")
	FVector NavProjectionExtent = FVector(650.0f, 650.0f, 1200.0f);

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventInitialized();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventCompleted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Region Event")
	void BP_OnRegionEventExpired();

private:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	int32 RegionId = INDEX_NONE;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_RegionEventData> EventData = nullptr;

	UPROPERTY(ReplicatedUsing = OnRep_RuntimeState, BlueprintReadOnly, Category = "Region Event", meta = (AllowPrivateAccess = "true"))
	EGPRegionEventRuntimeState RuntimeState = EGPRegionEventRuntimeState::Dormant;

	FTimerHandle AutoExpireTimerHandle;

	void SetRuntimeState(EGPRegionEventRuntimeState NewState);
	void ApplyRegionState(uint8 NewState) const;
	void SpawnConfiguredEnemies();
	FVector ResolveSpawnLocation(float ScatterRadius, bool& bOutProjected) const;
	void DispatchStatePresentation(EGPRegionEventRuntimeState PresentedState);

	UFUNCTION()
	void OnRep_RuntimeState(EGPRegionEventRuntimeState PreviousState);

	UFUNCTION()
	void HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
