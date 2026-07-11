#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_LandscapeTestInstructionActor.generated.h"

class UTextRenderComponent;

/** Native instruction sign kept stable across unattended map regeneration. */
UCLASS()
class PROJECT_EDEN_API AGP_LandscapeTestInstructionActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_LandscapeTestInstructionActor();

	/** Updates the short play-test instructions displayed above the central deck. */
	void SetInstructionText(const FText& InText);

private:
	/** Native ownership prevents commandlet reloads from losing the presentation component. */
	UPROPERTY(VisibleAnywhere, Category = "Landscape Test Environment")
	TObjectPtr<UTextRenderComponent> InstructionText;
};
