#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventTestTriggerActor.generated.h"

class AGP_PlayerCharacter;
class AGP_RegionEventActor;
class UGP_RegionEventData;
class USphereComponent;

/**
 * Editor/PIE smoke-test helper that starts one Region Event without requiring GameMode zone flow.
 * Use this for content checks; production runs should still use AGP_RegionEventDirector.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventTestTriggerActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventTestTriggerActor();

	UFUNCTION(BlueprintCallable, Category = "Region Event|Test")
	AGP_RegionEventActor* TriggerRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event|Test")
	void CompleteActiveRegionEvent();

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	AGP_RegionEventActor* GetActiveRegionEvent() const { return ActiveEvent; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TObjectPtr<UGP_RegionEventData> EventData;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TSubclassOf<AGP_RegionEventActor> FallbackEventActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	int32 TestRegionId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	bool bTriggerOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	bool bTriggerOnPlayerOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	bool bAutoActivateSpawnedEvent = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test", meta = (ClampMin = "50.0", Units = "cm"))
	float TriggerRadius = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test", meta = (Units = "cm"))
	FVector EventSpawnOffset = FVector(0.0f, 0.0f, 60.0f);

private:
	UFUNCTION()
	void HandleTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSpawnedEventStateChanged(AGP_RegionEventActor* EventActor);

	void TriggerRegionEventNextTick();

private:
	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventActor> ActiveEvent;
};
