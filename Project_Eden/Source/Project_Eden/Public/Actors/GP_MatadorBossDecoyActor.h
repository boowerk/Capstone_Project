#pragma once

#include "CoreMinimal.h"
#include "Characters/GP_EnemyCharacter.h"
#include "GP_MatadorBossDecoyActor.generated.h"

class UCapsuleComponent;
class UGP_MatadorDecoyPressureComponent;
class UGP_MatadorBossStateComponent;
class UAnimInstance;
class UNiagaraSystem;
class USkeletalMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_MatadorBossDecoyActor : public AGP_EnemyCharacter
{
	GENERATED_BODY()

public:
	AGP_MatadorBossDecoyActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeDecoy(AActor* InMainBossActor, UGP_MatadorBossStateComponent* InStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBreakPresentation();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBullRedirectPresentation(AActor* BullActor, AActor* RedirectTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBullReturnPresentation(int32 ChainBreakCount, int32 ChainBreakTarget);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Rapier")
	void HandleRapierAimStarted(float InAimDuration);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Rapier")
	void HandleRapierDirectionLocked(FVector LockedDirection, float InCommitDelay);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Rapier")
	void HandleRapierThrust(FVector LockedDirection);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Cape")
	void HandleCapePrepareStarted(float InPrepareDuration);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Cape")
	void HandleCapeDirectionLocked(FVector LockedDirection);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador|Cape")
	void HandleCapeGustBurst(FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount);

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetMainBossActor() const { return MainBossActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	UGP_MatadorBossStateComponent* GetMatadorStateComponent() const { return MatadorStateComponent.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	UGP_MatadorDecoyPressureComponent* GetPressureComponent() const { return PressureComponent; }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	USkeletalMeshComponent* GetDecoyMesh() const;

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	bool IsMatadorBossDecoy() const { return bMatadorBossDecoy; }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void UpdateAnimationSet() override;
	virtual void HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyBroken();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyVanishRequested(float VanishDelay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyRedirectedBull(AActor* BullActor, AActor* RedirectTargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyBullReturned(int32 ChainBreakCount, int32 ChainBreakTarget);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnDecoyRapierAimStarted(float InAimDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnDecoyRapierDirectionLocked(FVector LockedDirection, float InCommitDelay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Rapier")
	void BP_OnDecoyRapierThrust(FVector LockedDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnDecoyCapePrepareStarted(float InPrepareDuration);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnDecoyCapeDirectionLocked(FVector LockedDirection);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador|Cape")
	void BP_OnDecoyCapeGustBurst(FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount);

private:
	void ApplyDefaultVisualLayout();
	void ApplyAnimationSetToDecoyMesh();
	class UGP_MatadorDecoyAnimInstance* GetDecoyAnimInstance() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_MatadorDecoyPressureComponent> PressureComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> DecoyMesh;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> MainBossActor;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_MatadorBossStateComponent> MatadorStateComponent;

	UPROPERTY(EditDefaultsOnly, Category = "Boss|Matador")
	float BrokenLifeSpan = 0.75f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraSystem> DecoyVanishEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector DecoyVanishEffectOffset = FVector(0.0f, 0.0f, 80.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|VFX", meta = (AllowPrivateAccess = "true"))
	FVector DecoyVanishEffectScale = FVector(1.35f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyCapsuleRadius = 34.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Visual", meta = (AllowPrivateAccess = "true", ClampMin = "0.0", Units = "cm"))
	float DecoyCapsuleHalfHeight = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Visual", meta = (AllowPrivateAccess = "true", Units = "cm"))
	FVector DecoyMeshRelativeLocation = FVector(0.0f, 0.0f, -100.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador|Visual", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<UAnimInstance> DecoyAnimClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	bool bMatadorBossDecoy = true;
};
