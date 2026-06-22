#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_DarkKnightChargeActor.generated.h"

class AGP_DarkArmorKnightBossCharacter;
class UGameplayEffect;
class USphereComponent;
class UStaticMeshComponent;

/** Server-owned coordinator moves the boss itself so the visible body and charge hit volume stay aligned. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_DarkKnightChargeActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_DarkKnightChargeActor();
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeCharge(AGP_DarkArmorKnightBossCharacter* InBoss, AActor* InTarget);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Dark Knight")
	void BP_OnChargeTelegraphStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Dark Knight")
	void BP_OnChargeStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Dark Knight")
	void BP_OnChargeFinished(bool bHitTarget);

private:
	void StartCharge();
	void FinishCharge(bool bHitTarget);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> TelegraphMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm/s"))
	float ChargeSpeed = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float MaxChargeDistance = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float HitRadius = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 1.3f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	TObjectPtr<AGP_DarkArmorKnightBossCharacter> Boss;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Target;

	FVector ChargeDirection = FVector::ForwardVector;
	float DistanceTravelled = 0.0f;
	FTimerHandle TelegraphTimerHandle;
	bool bChargeActive = false;
	bool bFinished = false;
};
