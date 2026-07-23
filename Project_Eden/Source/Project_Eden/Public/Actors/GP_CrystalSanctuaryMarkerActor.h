#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CrystalSanctuaryMarkerActor.generated.h"

class USphereComponent;
class UGameplayEffect;
class UGP_VisualCueComponent;
class UNiagaraSystem;
class UStaticMesh;
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
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayExplosionVFX(const FVector& ExplosionLocation);

	void Explode();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> VisualCueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DamageArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MarkerMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> MarkerStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|Visual", meta = (AllowPrivateAccess = "true"))
	FVector MarkerMeshScale = FVector(4.4f, 4.4f, 0.05f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> TelegraphVFX;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> ExplosionVFX;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	FVector TelegraphVFXScale = FVector(2.2f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	FVector ExplosionVFXScale = FVector(2.0f);

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
