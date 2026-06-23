#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GP_IceMistArea.generated.h"

class UGameplayEffect;
class UGP_SkillData;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class USphereComponent;

UCLASS()
class PROJECT_EDEN_API AGP_IceMistArea : public AActor
{
	GENERATED_BODY()

public:
	AGP_IceMistArea();

	void InitializeIceMist(
		AActor* InSourceActor,
		UGP_SkillData* InSkillData,
		TSubclassOf<UGameplayEffect> InDamageEffectClass,
		TSubclassOf<UGameplayEffect> InSlowEffectClass,
		FGameplayTag InHitEventTag,
		float InEffectLevel,
		float InRadius,
		float InDuration,
		float InDamageInterval,
		const FVector& InLaunchDirection,
		float InLaunchSpeed,
		float InMoveDuration,
		bool bInDrawDebug);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Ice Mist")
	TObjectPtr<USphereComponent> MovementCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Ice Mist")
	TObjectPtr<USphereComponent> AreaCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Eden|Ice Mist")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	UFUNCTION()
	void HandleProjectileStopped(const FHitResult& ImpactResult);

	UFUNCTION()
	void OnRep_MovementStopped();

	void StopMistMovement();
	void ApplyStoppedMovementState();

	UFUNCTION()
	void HandleAreaBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UFUNCTION()
	void HandleAreaEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex);

	void ApplyDamageTick();
	void AddAffectedActor(AActor* OtherActor);
	void RemoveAffectedActor(AActor* OtherActor);
	void RemoveAllSlowEffects();

	UPROPERTY(Transient)
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient)
	TObjectPtr<UGP_SkillData> SkillData;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> SlowEffectClass;

	FGameplayTag HitEventTag;
	float EffectLevel = 1.0f;
	float AreaRadius = 300.0f;
	float AreaDuration = 5.0f;
	float DamageInterval = 0.5f;
	FVector LaunchDirection = FVector::ForwardVector;
	float LaunchSpeed = 600.0f;
	float MoveDuration = 0.5f;
	bool bDrawDebug = false;

	UPROPERTY(ReplicatedUsing = OnRep_MovementStopped)
	bool bMovementStopped = false;

	TMap<TWeakObjectPtr<AActor>, FActiveGameplayEffectHandle> SlowEffectHandles;
	FTimerHandle DamageTimerHandle;
	FTimerHandle MovementTimerHandle;
};
