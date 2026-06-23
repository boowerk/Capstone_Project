#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "GP_PeriodicAreaDamageActor.generated.h"

class UGameplayEffect;
class UGP_SkillData;
class USceneComponent;

UCLASS()
class PROJECT_EDEN_API AGP_PeriodicAreaDamageActor : public AActor
{
	GENERATED_BODY()

public:
	AGP_PeriodicAreaDamageActor();

	void InitializePeriodicDamage(
		AActor* InSourceActor,
		UGP_SkillData* InSkillData,
		TSubclassOf<UGameplayEffect> InDamageEffectClass,
		FGameplayTag InHitEventTag,
		float InEffectLevel,
		float InRadius,
		float InDuration,
		float InInterval,
		float InDamageScale,
		bool bInDrawDebug);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void ApplyDamageTick();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(Transient)
	TObjectPtr<AActor> SourceActor;

	UPROPERTY(Transient)
	TObjectPtr<UGP_SkillData> SkillData;

	UPROPERTY(Transient)
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	FGameplayTag HitEventTag;
	float EffectLevel = 1.0f;
	float Radius = 0.0f;
	float Duration = 0.0f;
	float Interval = 0.5f;
	float DamageScale = 0.25f;
	int32 MaxDamageTicks = 0;
	int32 AppliedDamageTicks = 0;
	bool bDrawDebug = false;
	FTimerHandle DamageTimerHandle;
};
