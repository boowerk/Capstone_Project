#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_LandscapeTestInstructionActor.generated.h"

class UTextRenderComponent;
class USceneComponent;

/** Native instruction sign kept stable across unattended map regeneration. */
UCLASS()
class PROJECT_EDEN_API AGP_LandscapeTestInstructionActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_LandscapeTestInstructionActor();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Updates the short play-test instructions displayed above the central deck. */
	void SetInstructionText(const FText& InText);

private:
	/** Repairs instances saved before the stable scene root was introduced. */
	void EnsureStableRoot();

	/** Transform root remains loadable even when commandlets exclude text-render components. */
	UPROPERTY(VisibleAnywhere, Category = "Landscape Test Environment")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Native ownership prevents commandlet reloads from losing the presentation component. */
	UPROPERTY(VisibleAnywhere, Category = "Landscape Test Environment")
	TObjectPtr<UTextRenderComponent> InstructionText;

	/** Persist the message independently so editor loads can rebuild stripped presentation state. */
	UPROPERTY(EditAnywhere, Category = "Landscape Test Environment")
	FText DisplayText;
};
