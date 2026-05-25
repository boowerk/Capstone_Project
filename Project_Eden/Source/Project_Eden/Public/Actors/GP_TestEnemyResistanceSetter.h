#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_TestEnemyResistanceSetter.generated.h"

class UGameplayEffect;

UCLASS()
class PROJECT_EDEN_API AGP_TestEnemyResistanceSetter : public AActor
{
	GENERATED_BODY()

public:
	AGP_TestEnemyResistanceSetter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistance Test")
	FName TargetTag = TEXT("TestResistanceTarget");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistance Test")
	TSubclassOf<UGameplayEffect> ResistanceEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistance Test", meta = (ClampMin = "0.0"))
	float EffectLevel = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Resistance Test", meta = (ClampMin = "0.0"))
	float InitialDelay = 0.2f;

private:
	void ApplyResistanceEffects();
};
