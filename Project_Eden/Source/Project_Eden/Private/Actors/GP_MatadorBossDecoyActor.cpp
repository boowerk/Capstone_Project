#include "Actors/GP_MatadorBossDecoyActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AGP_MatadorBossDecoyActor::AGP_MatadorBossDecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CollisionCapsule->InitCapsuleSize(34.0f, 100.0f);
	CollisionCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(CollisionCapsule);

	DecoyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DecoyMesh"));
	DecoyMesh->SetupAttachment(CollisionCapsule);
	DecoyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DecoyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));

	// SK_MaskMan is the requested default visual; designers can replace it in Blueprint per boss variant.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
	if (MaskManMeshFinder.Succeeded())
	{
		DecoyMesh->SetSkeletalMesh(MaskManMeshFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> VanishEffectFinder(TEXT("/Game/Imported_VFX/Free_Magic/VFX_Niagara/NS_Free_Magic_Hit2.NS_Free_Magic_Hit2"));
	if (VanishEffectFinder.Succeeded())
	{
		DecoyVanishEffect = VanishEffectFinder.Object;
	}
}

void AGP_MatadorBossDecoyActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyDefaultVisualLayout();

	if (HasAuthority() && IsValid(MatadorStateComponent.Get()))
	{
		MatadorStateComponent->RegisterDecoyActor(this);
	}
}

void AGP_MatadorBossDecoyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyDefaultVisualLayout();
}

void AGP_MatadorBossDecoyActor::ApplyDefaultVisualLayout()
{
	if (IsValid(CollisionCapsule))
	{
		CollisionCapsule->SetCapsuleSize(
			FMath::Max(1.0f, DecoyCapsuleRadius),
			FMath::Max(1.0f, DecoyCapsuleHalfHeight),
			true);
	}

	if (IsValid(DecoyMesh))
	{
		DecoyMesh->SetRelativeLocation(DecoyMeshRelativeLocation);
	}
}

void AGP_MatadorBossDecoyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_MatadorBossDecoyActor, MainBossActor);
	DOREPLIFETIME(AGP_MatadorBossDecoyActor, MatadorStateComponent);
}

UAbilitySystemComponent* AGP_MatadorBossDecoyActor::GetAbilitySystemComponent() const
{
	// Damage applied to the decoy intentionally resolves to the real boss ASC.
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MainBossActor.Get());
}

void AGP_MatadorBossDecoyActor::InitializeDecoy(AActor* InMainBossActor, UGP_MatadorBossStateComponent* InStateComponent)
{
	if (!HasAuthority())
	{
		return;
	}

	MainBossActor = InMainBossActor;
	MatadorStateComponent = InStateComponent;
	if (IsValid(MatadorStateComponent.Get()))
	{
		MatadorStateComponent->RegisterDecoyActor(this);
	}
}

void AGP_MatadorBossDecoyActor::PlayBreakPresentation()
{
	BP_OnDecoyBroken();
	BP_OnDecoyVanishRequested(FMath::Max(0.05f, BrokenLifeSpan));

	if (DecoyVanishEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			DecoyVanishEffect,
			GetActorLocation() + DecoyVanishEffectOffset,
			GetActorRotation(),
			DecoyVanishEffectScale,
			true,
			true,
			ENCPoolMethod::AutoRelease);
	}

	if (IsValid(DecoyMesh))
	{
		DecoyMesh->SetHiddenInGame(true);
		DecoyMesh->SetVisibility(false, true);
	}

	if (IsValid(CollisionCapsule))
	{
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (HasAuthority())
	{
		SetLifeSpan(FMath::Max(0.05f, BrokenLifeSpan));
	}
}

void AGP_MatadorBossDecoyActor::PlayBullRedirectPresentation(AActor* BullActor, AActor* RedirectTargetActor)
{
	BP_OnDecoyRedirectedBull(BullActor, RedirectTargetActor);
}

void AGP_MatadorBossDecoyActor::PlayBullReturnPresentation(int32 ChainBreakCount, int32 ChainBreakTarget)
{
	BP_OnDecoyBullReturned(ChainBreakCount, ChainBreakTarget);
}
