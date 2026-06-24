#include "Actors/GP_BigHammerDropActor.h"

#include "AbilitySystem/Abilities/GP_GameplayAbility.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"
#include "Characters/GP_BaseCharacter.h"
#include "Components/SceneComponent.h"
#include "Utils/GP_BlueprintLibrary.h"

AGP_BigHammerDropActor::AGP_BigHammerDropActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	bReplicates = true;
	SetReplicateMovement(true);
}

void AGP_BigHammerDropActor::InitializeDrop(
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
	bool bInDrawDebugs)
{
	if (!HasAuthority())
	{
		return;
	}

	SourceActor = InSourceActor;
	SkillData = InSkillData;
	DamageEffectClass = InDamageEffectClass;
	HitEventTag = InHitEventTag;
	EffectLevel = FMath::Max(1, InEffectLevel);
	DropEndLocation = InDropEndLocation;
	DamageImpactLocation = InDamageImpactLocation;
	ImpactRadius = FMath::Max(0.0f, InImpactRadius);
	DropDuration = FMath::Max(0.01f, InDropDuration);
	ImpactVisualActorClass = InImpactVisualActorClass;
	NiagaraParameterOverrides = InNiagaraParameterOverrides;
	bDrawDebugs = bInDrawDebugs;
	DropStartLocation = GetActorLocation();
	DropElapsedTime = 0.0f;
	bInitialized = true;

	SetActorTickEnabled(true);
	ForceNetUpdate();
}

void AGP_BigHammerDropActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!HasAuthority() || !bInitialized)
	{
		return;
	}

	DropElapsedTime += DeltaSeconds;
	const float Alpha = FMath::Clamp(DropElapsedTime / DropDuration, 0.0f, 1.0f);
	const float FallAlpha = Alpha * Alpha;
	SetActorLocation(FMath::Lerp(DropStartLocation, DropEndLocation, FallAlpha));

	if (Alpha >= 1.0f)
	{
		CompleteDrop();
	}
}

void AGP_BigHammerDropActor::CompleteDrop()
{
	bInitialized = false;
	SetActorTickEnabled(false);
	SetActorLocation(DropEndLocation);

	if (AGP_BaseCharacter* SourceCharacter = Cast<AGP_BaseCharacter>(SourceActor))
	{
		SourceCharacter->ShowSkillVisualActor(
			ImpactVisualActorClass,
			DamageImpactLocation,
			FRotator::ZeroRotator,
			1.0f,
			NiagaraParameterOverrides);
	}

	if (IsValid(SourceActor) && DamageEffectClass)
	{
		const TArray<AActor*> HitActors = UGP_BlueprintLibrary::SphereOverlapActorsAtLocation(
			SourceActor,
			DamageImpactLocation,
			ImpactRadius,
			SourceActor,
			bDrawDebugs && UGP_GameplayAbility::IsSkillDebugDrawEnabled());

		UGP_BlueprintLibrary::ApplyGameplayEffectAndEventToActors(
			SourceActor,
			HitActors,
			DamageEffectClass,
			HitEventTag,
			EffectLevel,
			SkillData);
	}

	Destroy();
}
