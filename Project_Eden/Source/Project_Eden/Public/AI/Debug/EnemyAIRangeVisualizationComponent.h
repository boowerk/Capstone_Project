#pragma once

#include "Components/PrimitiveComponent.h"
#include "CoreMinimal.h"
#include "EnemyAIRangeVisualizationComponent.generated.h"

UCLASS(ClassGroup = AI, hidecategories = (Collision, Cooking, LOD, Lighting, Navigation, Physics, Rendering, Tags))
class PROJECT_EDEN_API UEnemyAIRangeVisualizationComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UEnemyAIRangeVisualizationComponent();

	void ConfigureRanges(
		float InReturnHomeRadius,
		float InPatrolRadius,
		float InSightRadius,
		float InLoseSightRadius,
		float InPeripheralVisionAngleDegrees);

	void ConfigureVisibility(bool bInShowRanges, bool bInDrawOnlyIfSelected);

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;

private:
	friend class FEnemyAIRangeVisualizationSceneProxy;

	UPROPERTY()
	float ReturnHomeRadius = 2200.0f;

	UPROPERTY()
	float PatrolRadius = 1200.0f;

	UPROPERTY()
	float SightRadius = 2000.0f;

	UPROPERTY()
	float LoseSightRadius = 2400.0f;

	UPROPERTY()
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY()
	bool bDrawOnlyIfSelected = true;
};
