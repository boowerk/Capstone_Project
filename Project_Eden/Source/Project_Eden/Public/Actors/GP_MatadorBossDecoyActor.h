#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Actor.h"
#include "GP_MatadorBossDecoyActor.generated.h"

class UCapsuleComponent;
class UAbilitySystemComponent;
class UGP_MatadorBossStateComponent;
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

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	AActor* GetMainBossActor() const { return MainBossActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Boss|Matador")
	UGP_MatadorBossStateComponent* GetMatadorStateComponent() const { return MatadorStateComponent.Get(); }

protected:
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Boss|Matador")
	void BP_OnDecoyBroken();

private:
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
};
