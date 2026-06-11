#include "Actors/GP_PeriodicAreaDamageActor.h"

#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Components/SceneComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_PeriodicAreaDamageActor::AGP_PeriodicAreaDamageActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void AGP_PeriodicAreaDamageActor::InitializePeriodicDamage(
	AActor* InSourceActor,
	UGP_SkillData* InSkillData,
	TSubclassOf<UGameplayEffect> InDamageEffectClass,
	FGameplayTag InHitEventTag,
	float InEffectLevel,
	float InRadius,
	float InDuration,
	float InInterval,
	float InDamageScale,
	bool bInDrawDebug)
{
	SourceActor = InSourceActor;
	SkillData = InSkillData;
	DamageEffectClass = InDamageEffectClass;
	HitEventTag = InHitEventTag;
	EffectLevel = FMath::Max(InEffectLevel, 1.0f);
	Radius = FMath::Max(InRadius, 0.0f);
	Duration = FMath::Max(InDuration, 0.0f);
	Interval = FMath::Max(InInterval, 0.01f);
	DamageScale = FMath::Max(InDamageScale, 0.0f);
	MaxDamageTicks = FMath::FloorToInt((Duration - KINDA_SMALL_NUMBER) / Interval);
	bDrawDebug = bInDrawDebug;
}

void AGP_PeriodicAreaDamageActor::BeginPlay()
{
	Super::BeginPlay();

	if (!HasAuthority() || !IsValid(SourceActor) || !DamageEffectClass || Radius <= 0.0f || Duration <= 0.0f || DamageScale <= 0.0f)
	{
		Destroy();
		return;
	}

	if (MaxDamageTicks > 0)
	{
		GetWorldTimerManager().SetTimer(
			DamageTimerHandle,
			this,
			&ThisClass::ApplyDamageTick,
			Interval,
			true,
			Interval);
	}
	SetLifeSpan(Duration);
}

void AGP_PeriodicAreaDamageActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	Super::EndPlay(EndPlayReason);
}

void AGP_PeriodicAreaDamageActor::ApplyDamageTick()
{
	if (!IsValid(SourceActor))
	{
		Destroy();
		return;
	}

	if (AppliedDamageTicks >= MaxDamageTicks)
	{
		GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		return;
	}

	const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
		SourceActor,
		GetActorLocation(),
		Radius,
		SourceActor,
		bDrawDebug);

	UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
		SourceActor,
		HitActors,
		DamageEffectClass,
		HitEventTag,
		EffectLevel,
		SkillData,
		DamageScale);

	++AppliedDamageTicks;
	if (AppliedDamageTicks >= MaxDamageTicks)
	{
		GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	}
}
