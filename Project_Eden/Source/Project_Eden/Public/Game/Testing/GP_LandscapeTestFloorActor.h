#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_LandscapeTestFloorActor.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/** Flat test-only floor whose box collision is a deterministic NavMesh geometry source. */
UCLASS(NotBlueprintable)
class PROJECT_EDEN_API AGP_LandscapeTestFloorActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_LandscapeTestFloorActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	void ConfigureFloor(const FVector2D& InSize, float InThickness);

	UBoxComponent* GetNavigationFloor() const { return NavigationFloor; }

protected:
	UPROPERTY(VisibleAnywhere, Category = "Landscape Test Floor")
	TObjectPtr<UBoxComponent> NavigationFloor;

	UPROPERTY(VisibleAnywhere, Category = "Landscape Test Floor")
	TObjectPtr<UStaticMeshComponent> VisualMesh;

	UPROPERTY(EditAnywhere, Category = "Landscape Test Floor", meta = (ClampMin = "100.0", Units = "cm"))
	FVector2D FloorSize = FVector2D(1000.0f, 1000.0f);

	UPROPERTY(EditAnywhere, Category = "Landscape Test Floor", meta = (ClampMin = "10.0", Units = "cm"))
	float FloorThickness = 20.0f;

private:
	void RefreshFloorGeometry();
};
