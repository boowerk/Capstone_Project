#include "Actors/GP_DarkKnightChargeActor.h"

#include "Actors/GP_BossCombatUtils.h"
#include "Characters/GP_DarkArmorKnightBossCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayEffect.h"
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
	TelegraphMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("TelegraphMesh"));
	TelegraphMesh->SetupAttachment(SceneRoot);
	TelegraphMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TelegraphMesh->SetRelativeLocation(FVector(800.0f, 0.0f, 12.0f));
	TelegraphMesh->SetRelativeScale3D(FVector(16.0f, 3.6f, 0.04f));

	// Reusable designer component exposes its Niagara asset, scale, auto-activation, and lead time in Blueprint Details.
	ChargeTelegraphVFXComponent = CreateDefaultSubobject<UGP_BossTelegraphVFXComponent>(TEXT("ChargeTelegraphVFXComponent"));
	ChargeTelegraphVFXComponent->SetupAttachment(SceneRoot);
	// The legacy charge coordinator intentionally auto-plays its own cue; boss-level components remain opt-in.
	ChargeTelegraphVFXComponent->SetTelegraphVFXEnabled(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		TelegraphMesh->SetStaticMesh(CubeMeshFinder.Object);
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
	// Lightning lands during the existing telegraph delay; StartCharge remains gated by TelegraphDuration below.
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

void AGP_DarkKnightChargeActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!HasAuthority() || !bChargeActive || bFinished || !IsValid(Boss))
	{
		return;
	}

	if (bUseMontageRootMotion)
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
	ChargeElapsed = 0.0f;
	TelegraphMesh->SetVisibility(false, true);
	Boss->PlayPatternMontage(GPTags::Ability::Boss::DarkKnight::Charge);
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
