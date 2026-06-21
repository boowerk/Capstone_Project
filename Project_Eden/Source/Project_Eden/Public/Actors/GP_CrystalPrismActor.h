#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_CrystalPrismActor.generated.h"

class AGP_CrystalSeraphBossCharacter;
class AGP_SeraphLaserActor;
class UGP_VisualCueComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_CrystalPrismActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_CrystalPrismActor();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializePrism(AGP_CrystalSeraphBossCharacter* InBossOwner);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	bool NotifyLaserHit(AGP_SeraphLaserActor* LaserActor, const FVector& IncomingDirection);

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	FVector ResolveReflectedDirection(const FVector& IncomingDirection) const;

	UFUNCTION(BlueprintPure, Category = "Boss|Crystal Seraph")
	float GetCollisionRadius() const;

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnLaserReflected(AGP_SeraphLaserActor* LaserActor, const FVector& ReflectedDirection);

private:
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayReflectionVFX(const FVector& ReflectionLocation, const FRotator& ReflectionRotation);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_VisualCueComponent> VisualCueComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PrismMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> ReflectionCollision;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "s"))
	float PrismLifeSpan = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float ReflectionAngleDegrees = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CollisionRadius = 150.0f;

	// Keep the prototype crystal readable while allowing a Blueprint child to replace or retune its visual footprint.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	FVector PrismVisualScale = FVector(2.1f, 2.1f, 2.9f);

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AGP_CrystalSeraphBossCharacter> BossOwner;
};
