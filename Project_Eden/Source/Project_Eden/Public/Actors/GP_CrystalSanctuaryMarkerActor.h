#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CrystalSanctuaryMarkerActor.generated.h"

class USphereComponent;
class UGameplayEffect;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalSanctuaryMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_CrystalSanctuaryMarkerActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializeSanctuaryMarker(AActor* InBossOwner);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnTelegraphStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnExploded();

private:
	void Explode();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DamageArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float MarkerRadius = 220.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 1.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	TObjectPtr<AActor> BossOwner;

	FTimerHandle ExplosionTimerHandle;
};
