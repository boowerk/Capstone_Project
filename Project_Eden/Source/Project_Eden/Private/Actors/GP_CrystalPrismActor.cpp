#include "Actors/GP_CrystalPrismActor.h"

#include "Actors/GP_CrystalSeraphVFXDefaults.h"
#include "Actors/GP_SeraphLaserActor.h"
#include "Characters/GP_CrystalSeraphBossCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayTags/GP_Tags.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "VFX/GP_VisualCueComponent.h"

AGP_CrystalPrismActor::AGP_CrystalPrismActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorTickEnabled(false);
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	VisualCueComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("VisualCueComponent"));
	VisualCueComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());
	PrismShieldVFXComponent = CreateDefaultSubobject<UGP_VisualCueComponent>(TEXT("PrismShieldVFXComponent"));
	PrismShieldVFXComponent->SetNiagaraTintOverride(true, GPCrystalSeraphVFXDefaults::GetCrystalTintColor());

	ReflectionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("ReflectionCollision"));
	ReflectionCollision->SetupAttachment(SceneRoot);
	ReflectionCollision->SetSphereRadius(CollisionRadius);
	ReflectionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ReflectionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	ReflectionCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	ReflectionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PrismMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrismMesh"));
	PrismMesh->SetupAttachment(SceneRoot);
	PrismMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PrismShieldMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PrismShieldMesh"));
	PrismShieldMesh->SetupAttachment(SceneRoot);
	PrismShieldMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PrismShieldMesh->SetHiddenInGame(true);
}

void AGP_CrystalPrismActor::BeginPlay()
{
	Super::BeginPlay();

	// A mesh assigned directly on the Blueprint component has priority over the fallback property.
	if (PrismMesh->GetStaticMesh() == nullptr && IsValid(PrismStaticMesh.Get()))
	{
		PrismMesh->SetStaticMesh(PrismStaticMesh);
	}
	if (IsValid(PrismAuraVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, PrismAuraVFX);
	}
	if (IsValid(PrismReflectionVFX.Get()))
	{
		VisualCueComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Reflect_Magic, PrismReflectionVFX);
	}
	if (IsValid(PrismShieldVFX.Get()))
	{
		PrismShieldVFXComponent->AddNiagaraCue(GPTags::GameplayCue::Ability::Active_Magic, PrismShieldVFX);
	}
	// Apply the editable scale at runtime so native and Blueprint prism classes use the same collision-readable size.
	PrismMesh->SetRelativeScale3D(FVector(
		FMath::Max(0.01f, PrismVisualScale.X),
		FMath::Max(0.01f, PrismVisualScale.Y),
		FMath::Max(0.01f, PrismVisualScale.Z)));
	ReflectionCollision->SetSphereRadius(GetCollisionRadius());
	const FVector SafeAuraScale(
		FMath::Max(0.01f, PrismAuraScale.X),
		FMath::Max(0.01f, PrismAuraScale.Y),
		FMath::Max(0.01f, PrismAuraScale.Z));
	VisualCueComponent->ActivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic, PrismMesh, FVector::ZeroVector, FRotator::ZeroRotator, SafeAuraScale);
	ActivatePrismShield();
	SetLifeSpan(FMath::Max(0.0f, PrismLifeSpan));
}

void AGP_CrystalPrismActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePrismShieldVisual(DeltaSeconds);
}

void AGP_CrystalPrismActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_CrystalPrismActor, BossOwner);
}

void AGP_CrystalPrismActor::InitializePrism(AGP_CrystalSeraphBossCharacter* InBossOwner)
{
	BossOwner = InBossOwner;
	SetOwner(InBossOwner);
	SetInstigator(InBossOwner);
	UpdatePrismShieldDirection();
}

void AGP_CrystalPrismActor::OnRep_BossOwner()
{
	UpdatePrismShieldDirection();
}

void AGP_CrystalPrismActor::ActivatePrismShield()
{
	if (!IsValid(PrismShieldMesh))
	{
		return;
	}

	PrismShieldMesh->SetRelativeLocation(PrismShieldRelativeLocation);
	PrismShieldMesh->SetRelativeScale3D(PrismShieldVisualScale.ComponentMax(FVector(0.01f)));
	PrismShieldMesh->SetHiddenInGame(false);
	PrismShieldMaterialInstance = PrismShieldMesh->CreateDynamicMaterialInstance(0);
	if (IsValid(PrismShieldMaterialInstance))
	{
		PrismShieldMaterialInstance->SetScalarParameterValue(TEXT("Fade"), 0.0f);
		PrismShieldMaterialInstance->SetScalarParameterValue(TEXT("ReleaseProgress"), 0.0f);
	}

	bPrismShieldFadingIn = true;
	bPrismShieldReleasing = false;
	PrismShieldPhaseElapsed = 0.0f;
	ActivePrismShieldVFX = PrismShieldVFXComponent->ActivatePersistentCue(
		GPTags::GameplayCue::Ability::Active_Magic,
		PrismShieldMesh,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		FVector::OneVector);
	UpdatePrismShieldDirection();
	SetActorTickEnabled(true);
}

void AGP_CrystalPrismActor::BeginPrismShieldRelease()
{
	if (HasAuthority())
	{
		MulticastBeginPrismShieldRelease();
	}
}

void AGP_CrystalPrismActor::MulticastBeginPrismShieldRelease_Implementation()
{
	if (!IsValid(PrismShieldMesh) || PrismShieldMesh->bHiddenInGame)
	{
		return;
	}

	bPrismShieldFadingIn = false;
	bPrismShieldReleasing = true;
	PrismShieldPhaseElapsed = 0.0f;
	SetActorTickEnabled(true);
}

void AGP_CrystalPrismActor::UpdatePrismShieldVisual(float DeltaSeconds)
{
	if (!bPrismShieldFadingIn && !bPrismShieldReleasing)
	{
		// The Seraph can reposition while the prism remains, so keep its brighter shield edge aimed at the boss.
		if (IsValid(PrismShieldMesh) && !PrismShieldMesh->bHiddenInGame)
		{
			UpdatePrismShieldDirection();
		}
		else
		{
			SetActorTickEnabled(false);
		}
		return;
	}

	PrismShieldPhaseElapsed += FMath::Max(0.0f, DeltaSeconds);
	if (bPrismShieldFadingIn)
	{
		const float FadeAlpha = PrismShieldFadeInDuration <= KINDA_SMALL_NUMBER
			? 1.0f
			: FMath::Clamp(PrismShieldPhaseElapsed / PrismShieldFadeInDuration, 0.0f, 1.0f);
		if (IsValid(PrismShieldMaterialInstance))
		{
			PrismShieldMaterialInstance->SetScalarParameterValue(TEXT("Fade"), FadeAlpha);
		}
		if (FadeAlpha >= 1.0f)
		{
			bPrismShieldFadingIn = false;
			PrismShieldPhaseElapsed = 0.0f;
		}
	}
	else if (bPrismShieldReleasing)
	{
		const float ReleaseAlpha = PrismShieldReleaseDuration <= KINDA_SMALL_NUMBER
			? 1.0f
			: FMath::Clamp(PrismShieldPhaseElapsed / PrismShieldReleaseDuration, 0.0f, 1.0f);
		if (IsValid(PrismShieldMaterialInstance))
		{
			PrismShieldMaterialInstance->SetScalarParameterValue(TEXT("ReleaseProgress"), ReleaseAlpha);
		}
		if (ReleaseAlpha >= 1.0f)
		{
			PrismShieldVFXComponent->DeactivatePersistentCue(GPTags::GameplayCue::Ability::Active_Magic);
			PrismShieldMesh->SetHiddenInGame(true);
			bPrismShieldReleasing = false;
		}
	}

	UpdatePrismShieldDirection();
}

void AGP_CrystalPrismActor::UpdatePrismShieldDirection()
{
	if (!IsValid(BossOwner))
	{
		return;
	}

	const FVector SeraphDirection = (BossOwner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	if (SeraphDirection.IsNearlyZero())
	{
		return;
	}

	if (IsValid(PrismShieldMaterialInstance))
	{
		PrismShieldMaterialInstance->SetVectorParameterValue(
			TEXT("SeraphDirection"),
			FLinearColor(SeraphDirection.X, SeraphDirection.Y, SeraphDirection.Z, 0.0f));
	}
	if (IsValid(ActivePrismShieldVFX))
	{
		ActivePrismShieldVFX->SetVariableVec3(TEXT("User.SeraphDirection"), SeraphDirection);
	}
}

bool AGP_CrystalPrismActor::NotifyLaserHit(AGP_SeraphLaserActor* LaserActor, const FVector& IncomingDirection)
{
	if (!HasAuthority() || !IsValid(BossOwner))
	{
		return false;
	}

	const FVector ReflectedDirection = ResolveReflectedDirection(IncomingDirection);
	if (ReflectedDirection.IsNearlyZero())
	{
		return false;
	}

	// Every successful reflected laser breaks one wing-core stage; the third stage drops the boss into groggy.
	BossOwner->RequestWingCoreBreak();
	BP_OnLaserReflected(LaserActor, ReflectedDirection);
	MulticastPlayReflectionVFX(GetActorLocation(), ReflectedDirection.Rotation());
	return true;
}

void AGP_CrystalPrismActor::MulticastPlayReflectionVFX_Implementation(const FVector& ReflectionLocation, const FRotator& ReflectionRotation)
{
	if (IsValid(VisualCueComponent))
	{
		VisualCueComponent->PlayOneShotAtLocation(GPTags::GameplayCue::Ability::Reflect_Magic, ReflectionLocation, ReflectionRotation, PrismReflectionVFXScale);
	}
}

FVector AGP_CrystalPrismActor::ResolveReflectedDirection(const FVector& IncomingDirection) const
{
	if (IsValid(BossOwner))
	{
		return (BossOwner->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	}

	const FVector SafeIncoming = IncomingDirection.GetSafeNormal();
	if (SafeIncoming.IsNearlyZero())
	{
		return FVector::ZeroVector;
	}

	const FVector PrismNormal = GetActorForwardVector().GetSafeNormal();
	return FMath::GetReflectionVector(SafeIncoming, PrismNormal).GetSafeNormal();
}

float AGP_CrystalPrismActor::GetCollisionRadius() const
{
	return FMath::Max(0.0f, CollisionRadius);
}
