#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GP_BullChargeActor.generated.h"

class AGP_MatadorBossDecoyActor;
class UBoxComponent;
class UGameplayEffect;
class UGP_MatadorBossStateComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_BullChargeActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BullChargeActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeBullCharge(AActor* InTargetActor, AGP_MatadorBossDecoyActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent, const FVector& InChargeDirection);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RedirectTowardActor(AActor* NewTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void StartCharge();

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullChargeStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullChargeEnded(bool bHitDecoy);

private:
	UFUNCTION()
	void OnChargeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void FinishCharge(bool bHitDecoy);
	void DrawTelegraph() const;
	FVector ResolveDesiredDirection() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BullVisualMesh;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ChargeSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "deg/s"))
	float HomingTurnSpeedDegrees = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float TelegraphLeadTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float MaxChargeLifeSeconds = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float TelegraphLength = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	FGameplayTag PlayerHitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	float EffectLevel = 1.0f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TObjectPtr<AGP_MatadorBossDecoyActor> DecoyActor;

	UPROPERTY(Transient)
	TObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	FTimerHandle ChargeStartTimerHandle;
	FVector ChargeDirection = FVector::ForwardVector;
	bool bChargeStarted = false;
	bool bChargeFinished = false;
};
