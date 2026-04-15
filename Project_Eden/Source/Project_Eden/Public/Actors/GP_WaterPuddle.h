#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffect.h"
#include "Interfaces/GP_Summonable.h"

#include "GP_WaterPuddle.generated.h"

class UDecalComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class PROJECT_EDEN_API AGP_WaterPuddle : public AActor, public IGP_Summonable
{
	GENERATED_BODY()

public:
	AGP_WaterPuddle();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 패시브(E 스킬 쿨타임 중) 발동 시 플레이어 쪽으로 당겨오기 위한 함수
	UFUNCTION(BlueprintCallable, Category = "Eden|Puddle")
	void PullTowards(AActor* TargetActor, float PullSpeed);

	// 현재 장판 반경 반환 (Q 스킬에서 적을 찾을 때 사용)
	UFUNCTION(BlueprintPure, Category = "Eden|Puddle")
	float GetCurrentRadius() const { return CurrentRadius; }

	// 스킬 시전 시 스폰 직후 호출하여 이동 방향을 설정하는 함수
	UFUNCTION(BlueprintCallable, Category = "Eden|Puddle")
	void InitializeMovement(AActor* Caster);

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	
	// FadeIn
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle|Fade")
	bool bEnableFadeIn = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle|Fade", meta = (EditCondition = "bEnableFadeIn"))
	float FadeInDuration = 0.5f;

	// FadeOut
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle|Fade")
	bool bEnableFadeOut = true;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle|Fade", meta = (EditCondition = "bEnableFadeOut"))
	float FadeOutDuration = 1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Puddle")
	TObjectPtr<UDecalComponent> PuddleDecal;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DecalDynamicMaterial;

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
	float StartRadius = 550.0f;

	// 장판 최소 크기
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle")
	float EndRadius = 200.0f;

	// 장판의 기본 유지 시간 스킬 레벨에 따라 스폰 시 덮어씌울 수 있음 - 슝민
	UPROPERTY(BlueprintReadWrite, Category = "Eden|Puddle", meta = (ExposeOnSpawn = "true"))
	float BaseDuration = 10.0f;
	
	// 장판 진행 속력
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Eden|Puddle|Movement")
	float InitialForwardSpeed = 150.0f;

	// 목적지 좌표
	FVector DestinationLoc = FVector::ZeroVector;
	bool bIsMovingToDestination = false;

	// 장판 생성 시점
	float StartTime = 0.0f;

	// 장판 소멸 예정 시점 (클라이언트로 이것만 복제)
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_EndTime, Category = "Eden|Puddle")
	float EndTime = 0.0f;

	// 시작 시점의 반지름: 병합 처리시 현재 크기로 사용
	float StartingRadiusForLerp = 0.0f;

	// 현재 반지름: 서버/클라 각자 계산
	UPROPERTY(BlueprintReadOnly, Category = "Eden|Puddle")
	float CurrentRadius = 0.0f;

	// 흡수당하고 있는지 여부: 동시에 서로 흡수 방지
	bool bIsBeingAbsorbed = false;

	FTimerHandle PuddleUpdateTimer;
	
	// 당겨오기 보간
	float PullStartDistance = 0.0f;
	float PullMaxSpeed = 0.0f;

	// 타이머용 컬리전 업데이트 함수
	UFUNCTION()
	void UpdatePuddleState();

	// 충돌 융합 이벤트
	UFUNCTION()
	void OnPuddleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// EndTime이 클라이언트에 도달시 사용
	UFUNCTION()
	void OnRep_EndTime();

	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Puddle")
	void BP_OnAbsorbed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Puddle")
	void BP_OnEndTimeUpdated(float NewDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Eden|Puddle")
	void BP_OnFadeOutAndDestroy();

public: // IGP_Summonable 상속
	virtual AActor* GetSummonOwner() const override { return GetInstigator(); }

	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandPullTowards(AActor* TargetActor, float PullSpeed) override;

	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandAttackTarget() override;

	UFUNCTION(BlueprintCallable, Category = "Eden|Summon")
	virtual void CommandMoveToLocation(const FVector& TargetLocation, float MoveSpeed) override;
};
