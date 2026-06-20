#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GP_MatadorBossDecoyActor.generated.h"

class UCapsuleComponent;
class UAbilitySystemComponent;
class UGP_MatadorBossStateComponent;
class UNiagaraSystem;
class USkeletalMeshComponent;

UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_MatadorBossDecoyActor : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AGP_MatadorBossDecoyActor();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void InitializeDecoy(AActor* InMainBossActor, UGP_MatadorBossStateComponent* InStateComponent);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBreakPresentation();

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBullRedirectPresentation(AActor* BullActor, AActor* RedirectTargetActor);

	UFUNCTION(BlueprintCallable, Category = "Boss|Matador")
	void PlayBullReturnPresentation(int32 ChainBreakCount, int32 ChainBreakTarget);

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetMainBossActor() const { return MainBossActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	UGP_MatadorBossStateComponent* GetMatadorStateComponent() const { return MatadorStateComponent.Get(); }

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyBroken();

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyVanishRequested(float VanishDelay);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyRedirectedBull(AActor* BullActor, AActor* RedirectTargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyBullReturned(int32 ChainBreakCount, int32 ChainBreakTarget);

private:
	void ApplyDefaultVisualLayout();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|Matador", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionCapsule;

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
};
