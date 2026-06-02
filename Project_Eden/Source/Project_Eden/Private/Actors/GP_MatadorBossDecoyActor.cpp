#include "Actors/GP_MatadorBossDecoyActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGP_MatadorBossDecoyActor::AGP_MatadorBossDecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	CollisionCapsule->InitCapsuleSize(78.0f, 128.0f);
	CollisionCapsule->SetCollisionObjectType(ECC_Pawn);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(CollisionCapsule);

	DecoyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DecoyMesh"));
	DecoyMesh->SetupAttachment(CollisionCapsule);
	DecoyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// SK_MaskMan is the requested default visual; designers can replace it in Blueprint per boss variant.
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MaskManMeshFinder(TEXT("/Game/Characters/MaskMan/SK_MaskMan.SK_MaskMan"));
	if (MaskManMeshFinder.Succeeded())
	{
		DecoyMesh->SetSkeletalMesh(MaskManMeshFinder.Object);
	}
}

void AGP_MatadorBossDecoyActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority() && IsValid(MatadorStateComponent.Get()))
	{
		MatadorStateComponent->RegisterDecoyActor(this);
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

	if (HasAuthority())
	{
		SetLifeSpan(FMath::Max(0.05f, BrokenLifeSpan));
	}
}
