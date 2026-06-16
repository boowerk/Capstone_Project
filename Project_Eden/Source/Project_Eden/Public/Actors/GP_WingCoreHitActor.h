#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GP_WingCoreHitActor.generated.h"

class AGP_CrystalSeraphBossCharacter;
class UGP_CrystalSeraphStateComponent;
class USphereComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_WingCoreHitActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_WingCoreHitActor();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void InitializeWingCore(AGP_CrystalSeraphBossCharacter* InBossOwner, UGP_CrystalSeraphStateComponent* InStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Crystal Seraph")
	void SetCoreActive(bool bNewActive);

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Crystal Seraph")
	void BP_OnCoreActiveChanged(bool bNewActive);

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> CoreCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> CoreMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Crystal Seraph", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float CoreRadius = 160.0f;

	UPROPERTY(Transient)
	TObjectPtr<AGP_CrystalSeraphBossCharacter> BossOwner;

	UPROPERTY(Transient)
	TObjectPtr<UGP_CrystalSeraphStateComponent> StateComponent;
};
