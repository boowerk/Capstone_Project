#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "VFX/GP_NiagaraParameterOverride.h"
#include "GP_BigHammerDropActor.generated.h"

class UGameplayEffect;
class UGP_SkillData;
class USceneComponent;

UCLASS()
class PROJECT_EDEN_API AGP_BigHammerDropActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_BigHammerDropActor();

	void InitializeDrop(
		AActor* InSourceActor,
		UGP_SkillData* InSkillData,
		TSubclassOf<UGameplayEffect> InDamageEffectClass,
		FGameplayTag InHitEventTag,
		int32 InEffectLevel,
		const FVector& InDropEndLocation,
		const FVector& InDamageImpactLocation,
		float InImpactRadius,
		float InDropDuration,
		TSubclassOf<AActor> InImpactVisualActorClass,
		const TArray<FGP_NiagaraParameterOverride>& InNiagaraParameterOverrides,
		bool bInDrawDebugs);

protected:
	virtual void Tick(float DeltaSeconds) override;

private:
	void CompleteDrop();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<AActor> SourceActor;

	UPROPERTY()
	TObjectPtr<UGP_SkillData> SkillData;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY()
	TSubclassOf<AActor> ImpactVisualActorClass;

	TArray<FGP_NiagaraParameterOverride> NiagaraParameterOverrides;
	FGameplayTag HitEventTag;
	FVector DropStartLocation = FVector::ZeroVector;
	FVector DropEndLocation = FVector::ZeroVector;
	FVector DamageImpactLocation = FVector::ZeroVector;
	float ImpactRadius = 300.0f;
	float DropDuration = 0.4f;
	float DropElapsedTime = 0.0f;
	int32 EffectLevel = 1;
	bool bDrawDebugs = false;
	bool bInitialized = false;
};
