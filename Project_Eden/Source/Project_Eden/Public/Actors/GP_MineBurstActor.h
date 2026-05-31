#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GP_MineBurstActor.generated.h"

class UGameplayEffect;
class UGP_SkillData;
class UPrimitiveComponent;
class USphereComponent;

UCLASS()
class PROJECT_EDEN_API AGP_MineBurstActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_MineBurstActor();

	void SetSkillData(UGP_SkillData* InSkillData) { SkillData = InSkillData; }
	void SetImpactVisualActorClass(TSubclassOf<AActor> InImpactVisualActorClass) { ImpactVisualActorClass = InImpactVisualActorClass; }
	void ApplyExplosionRadiusMultiplier(float RadiusMultiplier);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Mine")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Mine")
	TObjectPtr<USphereComponent> TriggerComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|GAS", meta = (ClampMin = "0.0"))
	float ExplosionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Mine", meta = (ClampMin = "0.0"))
	float TriggerRadius = 120.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Mine", meta = (ClampMin = "0.0"))
	float ArmDelay = 0.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Mine", meta = (ClampMin = "0.0"))
	float MineLifeSpan = 8.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Visuals")
	TSubclassOf<AActor> ImpactVisualActorClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Eden|Debug")
	bool bDrawDebug = false;

	UPROPERTY(BlueprintReadWrite, Category = "Eden|GAS", meta = (ExposeOnSpawn = "true"))
	float EffectLevel = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Eden|GAS", meta = (ExposeOnSpawn = "true"))
	TObjectPtr<UGP_SkillData> SkillData;

	UFUNCTION()
	void OnTriggerOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastSpawnImpactVisual(const FVector& ImpactLocation, TSubclassOf<AActor> VisualActorClass, float VisualScale);

	void ArmMine();
	void Explode();

	UPROPERTY(Transient)
	bool bIsArmed = false;

	UPROPERTY(Transient)
	bool bHasExploded = false;

	UPROPERTY(Transient)
	float VisualScaleMultiplier = 1.0f;

	FTimerHandle ArmTimerHandle;
};
