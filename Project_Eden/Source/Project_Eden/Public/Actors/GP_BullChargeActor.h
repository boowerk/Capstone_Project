#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GP_BullChargeActor.generated.h"

class AGP_MatadorBossDecoyActor;
class UBoxComponent;
class UChildActorComponent;
class UGameplayEffect;
class UGP_MatadorBossStateComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EGPMatadorBullChargeState : uint8
{
	SpawnTelegraph UMETA(DisplayName = "Spawn Telegraph"),
	ChargingToDecoy UMETA(DisplayName = "Charging To Decoy"),
	CirclingDecoyToPlayer UMETA(DisplayName = "Circling Decoy To Player"),
	RedirectedByDecoyToPlayer UMETA(DisplayName = "Redirected By Decoy To Player"),
	ChargingToPlayer UMETA(DisplayName = "Charging To Player"),
	RedirectedByPlayerToDecoy UMETA(DisplayName = "Redirected By Player To Decoy"),
	HitDecoy UMETA(DisplayName = "Hit Decoy"),
	Failed UMETA(DisplayName = "Failed"),
	Finished UMETA(DisplayName = "Finished")
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_BullChargeActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BullChargeActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void Destroyed() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeBullCharge(AActor* InInitialTargetActor, AActor* InPlayerTargetActor, AGP_MatadorBossDecoyActor* InDecoyActor, UGP_MatadorBossStateComponent* InStateComponent, const FVector& InChargeDirection);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void RedirectTowardActor(AActor* NewTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	bool TryRedirectTowardDecoy(AActor* RedirectingActor);

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool CanRedirectTowardDecoy(AActor* RedirectingActor) const;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void StartCharge();

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	EGPMatadorBullChargeState GetChargeState() const { return ChargeState; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullChargeStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullChargeEnded(bool bHitDecoy);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullRedirected(AActor* RedirectingActor, AActor* NewTargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnBullSpawnPresentation();

private:
	UFUNCTION()
	void OnChargeOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void SetChargeState(EGPMatadorBullChargeState NewState);
	bool TryHandleDecoyProximity(const FVector* PreviousLocation = nullptr);
	AActor* ResolvePlayerTargetActor();
	bool StartDecoyCircleRedirect();
	void TickDecoyCircleRedirect(float DeltaSeconds);
	FVector EvaluateDecoyRedirectCurve(float Alpha) const;
	FVector EvaluateDecoyRedirectCurveTangent(float Alpha) const;
	bool TryRedirectTowardPlayerFromDecoy();
	bool CanRecordDecoyHit() const;
	bool IsActorRelatedToDecoy(AActor* Actor) const;
	bool IsBlockingHitSafeNearDecoy(const FHitResult& Hit) const;
	void ApplyBullImpactToActor(AActor* HitActor);
	void FinishCharge(bool bHitDecoy);
	void ClearRegisteredBullState();
	void DrawTelegraph() const;
	FVector ResolveDesiredDirection() const;
	bool BeginChargeImpact();

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> BullVisualMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UChildActorComponent> BullActorVisual;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "cm/s"))
	float ChargeSpeed = 1800.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "deg/s"))
	float HomingTurnSpeedDegrees = 180.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float TelegraphLeadTime = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "s"))
	float MaxChargeLifeSeconds = 5.0f;

	// Redirect 단계가 lifespan을 갱신해도 패턴 전체가 이 절대 상한을 넘지 않게 한다.
	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.1", Units = "s"))
	float MaximumTotalActionLifeSeconds = 18.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador", meta = (ClampMin = "0.0", Units = "cm"))
	float TelegraphLength = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	FGameplayTag PlayerHitEventTag;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	float EffectLevel = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float BullHitBaseDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float BullHitAttackPowerCoefficient = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0"))
	float BullHitToughnessDamage = 20.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0", Units = "cm/s"))
	float BullHitKnockbackHorizontalSpeed = 2200.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador|Damage", meta = (ClampMin = "0.0", Units = "cm/s"))
	float BullHitKnockbackVerticalSpeed = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true"))
	bool bRedirectTowardDecoyOnPlayerOverlap = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float RedirectMaxDistance = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float RedirectMaxAngleDegrees = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyRedirectRadius = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyReturnHitRadius = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "360.0", Units = "deg"))
	float DecoyRedirectArcDegrees = 110.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "cm"))
	float DecoyRedirectArcRadius = 700.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", Units = "deg/s"))
	float DecoyRedirectArcSpeedDegrees = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float DecoyRedirectTurnInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float PlayerRedirectTurnInterpSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float DecoyRedirectLookAheadAlpha = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float DecoyRedirectTangentWeight = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float DecoyRedirectOuterSideStrengthP1 = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", ClampMax = "1.0"))
	float DecoyRedirectOuterSideStrengthP2 = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "s"))
	float DecoyRedirectLifeExtensionSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", Units = "s"))
	float PlayerCounterLifeExtensionSeconds = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Matador|Counterplay", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyRedirectCollisionGraceRadius = 1200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> BullSpawnEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector BullSpawnEffectOffset = FVector(0.0f, 0.0f, 45.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true"))
	FVector BullSpawnEffectScale = FVector(1.6f);

	/** Applied only to this Bull's spawn Niagara instance; source Niagara asset remains reusable by other effects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true"))
	FLinearColor BullSpawnEffectColor = FLinearColor(1.0f, 0.02f, 0.0f, 1.0f);

	/** Prototype trajectory helpers. Disabled by default for normal gameplay. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Debug", meta = (AllowPrivateAccess = "true"))
	bool bShowDebugVisuals = false;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	EGPMatadorBullChargeState ChargeState = EGPMatadorBullChargeState::SpawnTelegraph;

	UPROPERTY(Transient)
	TObjectPtr<AActor> TargetActor;

	UPROPERTY(Transient)
	TObjectPtr<AActor> PlayerTargetActor;

	UPROPERTY(Transient)
	TObjectPtr<AGP_MatadorBossDecoyActor> DecoyActor;

	UPROPERTY(Transient)
	TObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	FTimerHandle ChargeStartTimerHandle;
	FVector ChargeDirection = FVector::ForwardVector;
	FVector DecoyCircleStartOffset = FVector::ForwardVector;
	FVector DecoyRedirectCurveP0 = FVector::ZeroVector;
	FVector DecoyRedirectCurveP1 = FVector::ZeroVector;
	FVector DecoyRedirectCurveP2 = FVector::ZeroVector;
	FVector DecoyRedirectCurveP3 = FVector::ZeroVector;
	FVector LockedRedirectDirection = FVector::ForwardVector;
	float DecoyCircleRadius = 560.0f;
	float DecoyCircleProgressDegrees = 0.0f;
	float DecoyCircleDirectionSign = 1.0f;
	float DecoyRedirectCurveAlpha = 0.0f;
	float DecoyRedirectCurveLength = 1000.0f;
	float TotalActionElapsedSeconds = 0.0f;
	bool bChargeStarted = false;
	bool bChargeFinished = false;
	bool bImpactHandled = false;
	bool bRedirectedByPlayer = false;
	bool bRedirectedByDecoy = false;
	TArray<TWeakObjectPtr<AActor>> BullImpactHitActors;
};
