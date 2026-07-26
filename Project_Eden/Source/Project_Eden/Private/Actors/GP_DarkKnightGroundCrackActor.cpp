#include "Actors/GP_DarkKnightGroundCrackActor.h"

#include "Actors/GP_BossCombatUtils.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffect.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_DarkKnightGroundCrackActor::AGP_DarkKnightGroundCrackActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DamageArea = CreateDefaultSubobject<USphereComponent>(TEXT("DamageArea"));
	DamageArea->SetupAttachment(SceneRoot);
	DamageArea->SetSphereRadius(CrackRadius);
	DamageArea->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	DamageArea->SetCollisionResponseToAllChannels(ECR_Ignore);
	DamageArea->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	WarningDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("WarningDecal"));
	WarningDecal->SetupAttachment(SceneRoot);
	WarningDecal->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
	WarningDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	WarningDecal->DecalSize = FVector(96.0f, CrackRadius, CrackRadius);
	WarningDecal->SetFadeScreenSize(0.001f);
	WarningDecal->SetSortOrder(20);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> TelegraphMaterialFinder(
		TEXT("/Game/Effects/M_EmissiveCircleTelegraph_Decal.M_EmissiveCircleTelegraph_Decal"));
	if (TelegraphMaterialFinder.Succeeded())
	{
		WarningDecal->SetDecalMaterial(TelegraphMaterialFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ExplosionVFXFinder(
		TEXT("/Game/Mixed_Magic_VFX_Pack/VFX/NS_Dark_Solo_Impact.NS_Dark_Solo_Impact"));
	if (ExplosionVFXFinder.Succeeded())
	{
		ExplosionVFX = ExplosionVFXFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_DarkKnightGroundCrackActor::BeginPlay()
{
	Super::BeginPlay();
	DamageArea->SetSphereRadius(FMath::Max(0.0f, CrackRadius));
	WarningDecal->DecalSize = FVector(96.0f, CrackRadius, CrackRadius);
	BP_OnTelegraphStarted();
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(ExplosionTimerHandle, this, &ThisClass::Explode, FMath::Max(0.01f, TelegraphDuration), false);
	}
}

void AGP_DarkKnightGroundCrackActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ExplosionTimerHandle);
	}
	Super::EndPlay(EndPlayReason);
}

void AGP_DarkKnightGroundCrackActor::InitializeGroundCrack(AActor* InBossOwner)
{
	SetOwner(InBossOwner);
	SetInstigator(Cast<APawn>(InBossOwner));
}

void AGP_DarkKnightGroundCrackActor::Explode()
{
	if (HasAuthority())
	{
		TArray<AActor*> Targets;
		DamageArea->GetOverlappingActors(Targets);
		GPBossCombatUtils::ApplyDamage(GetInstigator(), Targets, DamageEffectClass, AttackPowerDamageCoefficient, 0.0f, false, GPTags::Damage::Element::Brute);
	}
	if (IsValid(ExplosionVFX))
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			ExplosionVFX,
			GetActorLocation(),
			GetActorRotation());
	}
	BP_OnGroundCrackExploded();
	Destroy();
}
