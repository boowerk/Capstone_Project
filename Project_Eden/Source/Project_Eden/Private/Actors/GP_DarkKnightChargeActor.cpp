#include "Actors/GP_DarkKnightChargeActor.h"

#include "Actors/GP_BossCombatUtils.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Components/DecalComponent.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_BossTelegraphVFXComponent.h"

AGP_DarkKnightChargeActor::AGP_DarkKnightChargeActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	TelegraphDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("TelegraphDecal"));
	TelegraphDecal->SetupAttachment(SceneRoot);
	TelegraphDecal->SetRelativeLocation(FVector(MaxChargeDistance * 0.5f, 0.0f, 8.0f));
	TelegraphDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	// Local Z becomes actor forward after the downward rotation, preserving the authored 1600 x 360 warning lane.
	TelegraphDecal->DecalSize = FVector(96.0f, HitRadius, MaxChargeDistance * 0.5f);
	TelegraphDecal->SetFadeScreenSize(0.001f);
	TelegraphDecal->SetSortOrder(20);

	// Reusable designer component exposes its Niagara asset, scale, auto-activation, and lead time in Blueprint Details.
	ChargeTelegraphVFXComponent = CreateDefaultSubobject<UGP_BossTelegraphVFXComponent>(TEXT("ChargeTelegraphVFXComponent"));
	ChargeTelegraphVFXComponent->SetupAttachment(SceneRoot);
	// The legacy charge coordinator intentionally auto-plays its own cue; boss-level components remain opt-in.
	ChargeTelegraphVFXComponent->SetTelegraphVFXEnabled(true);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TelegraphMaterialFinder(
		TEXT("/Game/Effects/M_EmissiveCircleTelegraph_Decal.M_EmissiveCircleTelegraph_Decal"));
	if (TelegraphMaterialFinder.Succeeded())
	{
		TelegraphDecal->SetDecalMaterial(TelegraphMaterialFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ChargeVFXFinder(
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Lightning_Owner_Cast.NS_Lightning_Owner_Cast"));
	if (ChargeVFXFinder.Succeeded())
	{
		// This sprite-only system avoids exposing the source meshes used by the old example Niagara asset.
		ChargeTelegraphVFXComponent->SetDefaultTelegraphSystem(ChargeVFXFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_DarkKnightChargeActor::InitializeCharge(AGP_DarkArmorKnightBossCharacter* InBoss, AActor* InTarget)
{
	Boss = InBoss;
	Target = InTarget;
	if (!IsValid(Boss))
	{
		Destroy();
		return;
	}

	SetOwner(Boss);
	SetInstigator(Boss);
	const FVector TargetLocation = IsValid(Target) ? Target->GetActorLocation() : Boss->GetActorLocation() + Boss->GetActorForwardVector() * MaxChargeDistance;
	ChargeDirection = (TargetLocation - Boss->GetActorLocation()).GetSafeNormal2D();
	if (ChargeDirection.IsNearlyZero())
	{
		ChargeDirection = Boss->GetActorForwardVector().GetSafeNormal2D();
	}
	SetActorLocation(Boss->GetActorLocation());
	SetActorRotation(ChargeDirection.Rotation());
	Boss->SetActorRotation(ChargeDirection.Rotation());
	bSkipInternalTelegraph = Boss->IsBossTelegraphEnabledForPattern(GPTags::Ability::Boss::DarkKnight::Charge);
	if (bSkipInternalTelegraph)
	{
		// The boss-level cue already consumed its lead time before this coordinator was spawned.
		OnRep_SkipInternalTelegraph();
		ForceNetUpdate();
		StartCharge();
		return;
	}

	// With the boss-level toggle disabled, preserve the original coordinator-owned warning and delay.
	BP_OnChargeTelegraphStarted();

	if (UWorld* World = GetWorld())
	{
		const float TelegraphDuration = IsValid(ChargeTelegraphVFXComponent)
			? ChargeTelegraphVFXComponent->GetTelegraphDuration()
			: 0.9f;
		World->GetTimerManager().SetTimer(
			TelegraphTimerHandle,
			this,
			&ThisClass::StartCharge,
			FMath::Max(0.01f, TelegraphDuration),
			false);
	}
}

void AGP_DarkKnightChargeActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AGP_DarkKnightChargeActor, bSkipInternalTelegraph, COND_InitialOnly);
}

void AGP_DarkKnightChargeActor::OnRep_SkipInternalTelegraph()
{
	if (bSkipInternalTelegraph && IsValid(ChargeTelegraphVFXComponent))
	{
		// Clients may auto-register the component before initial actor state arrives, so suppress it on replication too.
		ChargeTelegraphVFXComponent->StopTelegraph();
		ChargeTelegraphVFXComponent->SetVisibility(false, true);
	}
	if (bSkipInternalTelegraph && IsValid(TelegraphDecal))
	{
		TelegraphDecal->SetVisibility(false, true);
	}
}

void AGP_DarkKnightChargeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !bChargeActive || bFinished || !IsValid(Boss))
	{
		return;
	}

	if (bActiveUsesMontageRootMotion)
	{
		ChargeElapsed += DeltaSeconds;
		SetActorLocation(Boss->GetActorLocation());
		if (IsValid(Target) && FVector::Dist2D(Boss->GetActorLocation(), Target->GetActorLocation()) <= HitRadius)
		{
			TArray<AActor*> Targets { Target };
			GPBossCombatUtils::ApplyDamage(Boss, Targets, DamageEffectClass, AttackPowerDamageCoefficient, 0.0f, false, GPTags::Damage::Element::Brute);
			FinishCharge(true);
			return;
		}

		if (ChargeElapsed >= RootMotionChargeDuration)
		{
			FinishCharge(false);
		}
		return;
	}

	const float Step = FMath::Min(FMath::Max(0.0f, ChargeSpeed) * DeltaSeconds, MaxChargeDistance - DistanceTravelled);
	FHitResult MoveHit;
	Boss->AddActorWorldOffset(ChargeDirection * Step, true, &MoveHit);
	DistanceTravelled += Step;
	SetActorLocation(Boss->GetActorLocation());

	if (IsValid(Target) && FVector::Dist2D(Boss->GetActorLocation(), Target->GetActorLocation()) <= HitRadius)
	{
		TArray<AActor*> Targets { Target };
		GPBossCombatUtils::ApplyDamage(Boss, Targets, DamageEffectClass, AttackPowerDamageCoefficient, 0.0f, false, GPTags::Damage::Element::Brute);
		FinishCharge(true);
		return;
	}

	if (MoveHit.bBlockingHit || DistanceTravelled >= MaxChargeDistance - KINDA_SMALL_NUMBER)
	{
		FinishCharge(false);
	}
}

void AGP_DarkKnightChargeActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TelegraphTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AGP_DarkKnightChargeActor::StartCharge()
{
	if (!HasAuthority() || !IsValid(Boss))
	{
		FinishCharge(false);
		return;
	}
	bChargeActive = true;
	DistanceTravelled = 0.0f;
	ChargeElapsed = 0.0f;
	if (IsValid(TelegraphDecal))
	{
		TelegraphDecal->SetVisibility(false, true);
	}
	const bool bMontageStarted = Boss->PlayPatternMontage(GPTags::Ability::Boss::DarkKnight::Charge);
	// Root motion can own travel only when this exact montage invocation started.
	bActiveUsesMontageRootMotion = bUseMontageRootMotion && bMontageStarted;
	SetActorTickEnabled(true);
	BP_OnChargeStarted();
}

void AGP_DarkKnightChargeActor::FinishCharge(bool bHitTarget)
{
	if (bFinished)
	{
		return;
	}
	bFinished = true;
	bChargeActive = false;
	SetActorTickEnabled(false);
	BP_OnChargeFinished(bHitTarget);
	if (HasAuthority() && IsValid(Boss))
	{
		Boss->HandleChargeFinished(bHitTarget, Target);
	}
	Destroy();
}
