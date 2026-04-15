#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "GP_Projectile.generated.h"

class UShapeComponent; 
class UProjectileMovementComponent;
class UGameplayEffect;

UCLASS()
class PROJECT_EDEN_API AGP_Projectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AGP_Projectile();

protected:
	virtual void BeginPlay() override;

	// 1. C++에서 기본으로 생성할 빈 루트 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<USceneComponent> RootScene;

	// 2. 블루프린트에서 추가한 콜리전을 C++에서 찾아 담아둘 포인터 (에디터 생성 X)
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UShapeComponent> CollisionComponent;

	// 투사체 무브먼트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// (이하 변수들은 이전과 동일)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	FGameplayTag HitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Eden|Projectile")
	float ProjectileLifeSpan = 5.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Eden|Projectile")
	bool bDestroyOnHit = true;

	UPROPERTY(BlueprintReadWrite, Category = "Eden|GAS", meta = (ExposeOnSpawn = "true"))
	float EffectLevel = 1.0f;

	// 이름 변경 (OnSphereOverlap -> OnProjectileOverlap)
	UFUNCTION()
	virtual void OnProjectileOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Projectile")
	void BP_OnHitEffect(const FVector& ImpactLocation);
};
