#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CorruptionTestZoneActor.generated.h"

class UBoxComponent;
class UPointLightComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EGPCorruptionTestScope : uint8
{
	World UMETA(DisplayName = "Whole World"),
	Region UMETA(DisplayName = "Single Region")
};

/** Walk-in PIE station that applies an exact corruption value and makes visual comparisons repeatable. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CorruptionTestZoneActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_CorruptionTestZoneActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "World Corruption|Test")
	bool ApplyTestCorruption();

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	float GetTestCorruption() const { return TestCorruption; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	UBoxComponent* GetTriggerBox() const { return TriggerBox; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	EGPCorruptionTestScope GetTestScope() const { return TestScope; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	bool IsApplyOnBeginPlayEnabled() const { return bApplyOnBeginPlay; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	bool IsApplyOnPlayerOverlapEnabled() const { return bApplyOnPlayerOverlap; }

	UFUNCTION(BlueprintPure, Category = "World Corruption|Test")
	bool ShouldPausePassiveIncrease() const { return bPausePassiveIncreaseWhenApplied; }

	/** Configures the deterministic world-level station used by the landscape smoke-test map. */
	void ConfigureWorldTestStation(float InCorruption, const FLinearColor& InStationColor);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	TObjectPtr<UBoxComponent> TriggerBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	TObjectPtr<UStaticMeshComponent> VisualPlatform;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	TObjectPtr<UTextRenderComponent> StationLabel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	TObjectPtr<UPointLightComponent> StationLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	EGPCorruptionTestScope TestScope = EGPCorruptionTestScope::World;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float TestCorruption = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test", meta = (EditCondition = "TestScope == EGPCorruptionTestScope::Region", ClampMin = "0"))
	int32 TestRegionId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	bool bApplyOnBeginPlay = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	bool bApplyOnPlayerOverlap = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	bool bPausePassiveIncreaseWhenApplied = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test", meta = (ClampMin = "100.0", Units = "cm"))
	FVector TriggerExtent = FVector(350.0f, 350.0f, 180.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "World Corruption|Test")
	FLinearColor StationColor = FLinearColor::Green;

	UFUNCTION(BlueprintImplementableEvent, Category = "World Corruption|Test")
	void BP_OnTestCorruptionApplied(float AppliedCorruption, int32 AppliedRegionId);

private:
	UFUNCTION()
	void HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void RefreshStationPresentation();
};
