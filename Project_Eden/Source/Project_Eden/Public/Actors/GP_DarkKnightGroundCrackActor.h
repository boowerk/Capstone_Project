#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_DarkKnightGroundCrackActor.generated.h"

class UGameplayEffect;
class USphereComponent;
class UStaticMeshComponent;

/** One adjustable crack marker owns both its telegraph and authoritative GAS explosion. */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_DarkKnightGroundCrackActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_DarkKnightGroundCrackActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void InitializeGroundCrack(AActor* InBossOwner);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Dark Knight")
	void BP_OnTelegraphStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Dark Knight")
	void BP_OnGroundCrackExploded();

private:
	void Explode();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> DamageArea;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CrackMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CrackRadius = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float TelegraphDuration = 1.4f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Dark Knight", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float AttackPowerDamageCoefficient = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Dark Knight|GAS", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	FTimerHandle ExplosionTimerHandle;
};
