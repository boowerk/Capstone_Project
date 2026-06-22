#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_SeraphLaserActor.generated.h"

class AGP_CrystalSeraphBossCharacter;
class UBoxComponent;
class UGameplayEffect;
class UGP_VisualCueComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_SeraphLaserActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_SeraphLaserActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializeLaser(AGP_CrystalSeraphBossCharacter* InBossOwner, AActor* InTargetActor, const FVector& InDirection, int32 InRemainingReflections = 1);

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	FVector GetLaserDirection() const { return LaserDirection; }

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetLaserLength() const { return FMath::Max(0.0f, LaserLength); }

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnLaserTelegraphStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnLaserActivated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnLaserReflected(const FVector& ReflectionOrigin, const FVector& ReflectedDirection);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayReflectedVFX(const FVector& ReflectionOrigin, const FRotator& ReflectionRotation);

	void ActivateLaser();
	void FinishLaser();
	void ApplyLaserDamageTick();
	bool TryReflectFromPrism();
	void SpawnReflectedSegment(const FVector& ReflectionOrigin, const FVector& ReflectedDirection);
	void UpdateLaserShape();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> VisualCueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> DamageBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LaserMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float ActiveDuration = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float LaserLength = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float LaserWidth = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.01", Units = "s"))
	float DamageTickInterval = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float MaxHealthDamagePerTick = 0.06f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Crystal Seraph|GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	TObjectPtr<AGP_CrystalSeraphBossCharacter> BossOwner;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	FVector LaserDirection = FVector::ForwardVector;
	int32 RemainingReflections = 1;
	FTimerHandle ActivationTimerHandle;
	FTimerHandle DamageTimerHandle;
	FTimerHandle EndTimerHandle;
	bool bLaserActive = false;
	bool bHasReflected = false;
};
