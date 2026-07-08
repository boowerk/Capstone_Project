#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_RegionEventCrystalNode.generated.h"

class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FGPOnRegionEventCrystalDestroyed, class AGP_RegionEventCrystalNode*, CrystalNode, AActor*, DestroyingActor);

/** Simple destructible crystal objective used by the crystal-corruption region event. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_RegionEventCrystalNode : public AActor
{
	GENERATED_BODY()

public:
	AGP_RegionEventCrystalNode();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Region Event|Crystal")
	void ApplyRegionEventHit(AActor* HitInstigator, float HitDamage = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Region Event|Crystal")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Region Event|Crystal")
	float GetMaxHealth() const { return MaxHealth; }

	UPROPERTY(BlueprintAssignable, Category = "Region Event|Crystal")
	FGPOnRegionEventCrystalDestroyed OnCrystalNodeDestroyed;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal")
	TObjectPtr<UStaticMeshComponent> CrystalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal", meta = (ClampMin = "1.0"))
	float MaxHealth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Region Event|Crystal")
	FVector VisualScale = FVector(1.0f, 1.0f, 2.4f);

private:
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Region Event|Crystal", meta = (AllowPrivateAccess = "true"))
	float CurrentHealth = 0.0f;

	bool bDestroyed = false;

	void DestroyCrystal(AActor* DestroyingActor);
};
