#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "Interfaces/GP_Summonable.h"

#include "GP_WaterPuddle.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class PROJECT_EDEN_API AGP_WaterPuddle : public AActor , public IGP_Summonable
{
	GENERATED_BODY()
	
public:	
	AGP_WaterPuddle();

	virtual void Tick(float DeltaTime) override;

	// 패시브(E 스킬 쿨타임 중) 발동 시 플레이어 쪽으로 당겨오기 위한 함수
	UFUNCTION(BlueprintCallable, Category = "Eden|Puddle")
	void PullTowards(AActor* TargetActor, float PullSpeed);

	// 현재 장판 반경 반환 (Q 스킬에서 적을 찾을 때 사용)
	UFUNCTION(BlueprintPure, Category = "Eden|Puddle")
	float GetCurrentRadius() const { return CurrentRadius; }

protected:
	virtual void BeginPlay() override;

	// 크기 변하는 장판 컬리전
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Puddle")
	TObjectPtr<USphereComponent> PuddleCollision;

	// 장판이 부드럽게 당겨질 때 쓸 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Puddle")
	TObjectPtr<UProjectileMovementComponent> MovementComponent;

	// 장판 내부에 들어온 적에게 걸 디버프 : 슬로우, 취약
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|GAS")
	TSubclassOf<UGameplayEffect> PuddleDebuffEffectClass;

	
	// 장판 최대 크기 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle")
	float MaxRadius = 350.0f;

	// 장판 최소 크기 
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle")
	float MinRadius = 80.0f;

	// 초당 줄어드는 반지름(나중에 지속시간 비례로 변경할 예정 - 슝민)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle")
	float ShrinkRatePerSecond = 15.0f;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Eden|Puddle")
	float CurrentRadius;

	// 흡수당하고 있는지 여부 (동시에 서로 흡수 방지)
	bool bIsBeingAbsorbed = false;

	// 충돌 융합 이벤트
	UFUNCTION()
	void OnPuddleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// 장판이 흡수될 때의 연출용 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Puddle")
	void BP_OnAbsorbed();

	// 장판이 최소 크기에 도달해 사라질 때 연출용 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Puddle")
	void BP_OnFadeOutAndDestroy();
	
public: // IGP_Summonable 상속
	virtual AActor* GetSummonOwner() const override { return GetInstigator(); }

	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandPullTowards(AActor* TargetActor, float PullSpeed) override;

	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandAttackTarget() override;
};
