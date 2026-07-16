#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventTestTriggerActor.generated.h"

class AGP_PlayerCharacter;
class AGP_RegionEventActor;
class UGP_RegionEventData;
class UPointLightComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

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

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, Category = "Region Event|Test")
	AGP_RegionEventActor* TriggerRegionEvent();

	UFUNCTION(BlueprintCallable, Category = "Region Event|Test")
	void CompleteActiveRegionEvent();

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	AGP_RegionEventActor* GetActiveRegionEvent() const { return ActiveEvent; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	UGP_RegionEventData* GetEventData() const { return EventData; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	int32 GetTestRegionId() const { return TestRegionId; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	bool IsTriggerOnBeginPlay() const { return bTriggerOnBeginPlay; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	bool IsTriggerOnPlayerOverlap() const { return bTriggerOnPlayerOverlap; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Test")
	FText GetStationTitle() const { return StationTitle; }

	/** Converts a generated one-off trigger BP into a walk-in station for the shared landscape test map. */
	void ConfigureAsOverlapTestStation(int32 InRegionId);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test")
	TObjectPtr<USphereComponent> TriggerSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	TObjectPtr<UStaticMeshComponent> VisualPlatform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	TObjectPtr<UTextRenderComponent> StationLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	TObjectPtr<UPointLightComponent> StationLight;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	FText StationTitle = FText::FromString(TEXT("REGION EVENT TEST"));

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	FLinearColor StationColor = FLinearColor(1.0f, 0.35f, 0.05f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Test|Presentation")
	bool bShowStationVisuals = true;

private:
	UFUNCTION()
	void HandleTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void HandleSpawnedEventStateChanged(AGP_RegionEventActor* EventActor);

	void TriggerRegionEventNextTick();
	void RefreshStationPresentation();

private:
	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventActor> ActiveEvent;
};
