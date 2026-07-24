#include "Actors/GP_MatadorBossDecoyActor.h"

#include "Actors/GP_MatadorDecoyPressureComponent.h"
#include "Animation/GP_MatadorDecoyAnimInstance.h"
#include "Animation/PDA_EnemyAnimationSet.h"
#include "Characters/GP_MatadorBossStateComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "VFX/GP_EnemyDeathAbsorptionComponent.h"

AGP_MatadorBossDecoyActor::AGP_MatadorBossDecoyActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	PressureComponent = CreateDefaultSubobject<UGP_MatadorDecoyPressureComponent>(TEXT("PressureComponent"));

	DecoyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("DecoyMesh"));
	DecoyMesh->SetupAttachment(GetCapsuleComponent());
	DecoyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DecoyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -100.0f));
	DecoyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetVisibility(false, true);

	bIsBossEnemy = false;
	bGrantDefaultBossPatternAbilities = false;
	bGrantDefaultEnemyAttackAbility = false;
	bGrantDefaultEnemyDeathAbility = true;
	bShowWorldHealthBar = false;
	XPReward = 0.0f;
	DeathDespawnDelay = 0.75f;
	if (UGP_EnemyDeathAbsorptionComponent* DeathAbsorptionComponent = GetEnemyDeathAbsorptionComponent())
	{
		// The decoy owns a separate visible mesh and bespoke break effect; it is
		// not a defeated monster reward and must not fly toward a player.
		DeathAbsorptionComponent->SetDeathAbsorptionEnabled(false);
	}
	AutoPossessAI = EAutoPossessAI::Disabled;
	AIControllerClass = nullptr;

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->InitCapsuleSize(34.0f, 100.0f);
		Capsule->SetCollisionObjectType(ECC_WorldDynamic);
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		Capsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		Capsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->MaxWalkSpeed = 220.0f;
		MovementComponent->bOrientRotationToMovement = false;
		MovementComponent->bUseControllerDesiredRotation = false;
	}

	static ConstructorHelpers::FObjectFinder<UPDA_EnemyAnimationSet> DecoyAnimationSetFinder(TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_Matador/Animation/PDA_MatadorDecoyAnimationSet.PDA_MatadorDecoyAnimationSet"));
	if (DecoyAnimationSetFinder.Succeeded())
	{
		EnemyAnimationSet = DecoyAnimationSetFinder.Object;
	}

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

	if (IsValid(PressureComponent))
	{
		PressureComponent->InitializePressure(MainBossActor.Get(), MatadorStateComponent.Get());
	}
}

void AGP_MatadorBossDecoyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyDefaultVisualLayout();
}

void AGP_MatadorBossDecoyActor::ApplyDefaultVisualLayout()
{
	if (USkeletalMeshComponent* InheritedMesh = GetMesh())
	{
		InheritedMesh->SetHiddenInGame(true);
		InheritedMesh->SetVisibility(false, true);
		InheritedMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		InheritedMesh->SetGenerateOverlapEvents(false);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCapsuleSize(
			FMath::Max(1.0f, DecoyCapsuleRadius),
			FMath::Max(1.0f, DecoyCapsuleHalfHeight),
			true);
	}

	if (USkeletalMeshComponent* MeshComponent = GetDecoyMesh())
	{
		MeshComponent->SetRelativeLocation(DecoyMeshRelativeLocation);
		MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		if (IsValid(EnemyAnimationSet))
		{
			ApplyAnimationSetToDecoyMesh();
		}
		else if (DecoyAnimClass)
		{
			MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
			MeshComponent->SetAnimInstanceClass(DecoyAnimClass);
		}
	}

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	GetComponents<UStaticMeshComponent>(StaticMeshComponents);
	for (UStaticMeshComponent* StaticMeshComponent : StaticMeshComponents)
	{
		if (IsValid(StaticMeshComponent))
		{
			StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			StaticMeshComponent->SetGenerateOverlapEvents(false);
		}
	}
}

void AGP_MatadorBossDecoyActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_MatadorBossDecoyActor, MainBossActor);
	DOREPLIFETIME(AGP_MatadorBossDecoyActor, MatadorStateComponent);
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
	if (IsValid(PressureComponent))
	{
		PressureComponent->InitializePressure(MainBossActor.Get(), MatadorStateComponent.Get());
		PressureComponent->StartPressure();
	}
}

void AGP_MatadorBossDecoyActor::PlayBreakPresentation()
{
	if (IsValid(PressureComponent))
	{
		PressureComponent->StopPressure();
	}

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

	if (USkeletalMeshComponent* MeshComponent = GetDecoyMesh())
	{
		MeshComponent->SetHiddenInGame(true);
		MeshComponent->SetVisibility(false, true);
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
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

void AGP_MatadorBossDecoyActor::HandleRapierAimStarted(float InAimDuration)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyRapierAimStarted(InAimDuration);
	}
	BP_OnDecoyRapierAimStarted(InAimDuration);
}

void AGP_MatadorBossDecoyActor::HandleRapierDirectionLocked(FVector LockedDirection, float InCommitDelay)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyRapierDirectionLocked(LockedDirection, InCommitDelay);
	}
	BP_OnDecoyRapierDirectionLocked(LockedDirection, InCommitDelay);
}

void AGP_MatadorBossDecoyActor::HandleRapierThrust(FVector LockedDirection)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyRapierThrust(LockedDirection);
	}
	BP_OnDecoyRapierThrust(LockedDirection);
}

void AGP_MatadorBossDecoyActor::HandleCapePrepareStarted(float InPrepareDuration)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyCapePrepareStarted(InPrepareDuration);
	}
	BP_OnDecoyCapePrepareStarted(InPrepareDuration);
}

void AGP_MatadorBossDecoyActor::HandleCapeDirectionLocked(FVector LockedDirection)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyCapeDirectionLocked(LockedDirection);
	}
	BP_OnDecoyCapeDirectionLocked(LockedDirection);
}

void AGP_MatadorBossDecoyActor::HandleCapeGustBurst(FVector LockedDirection, int32 InBurstIndex, int32 InBurstCount)
{
	if (UGP_MatadorDecoyAnimInstance* AnimInstance = GetDecoyAnimInstance())
	{
		AnimInstance->NotifyCapeGustBurst(LockedDirection, InBurstIndex, InBurstCount);
	}
	BP_OnDecoyCapeGustBurst(LockedDirection, InBurstIndex, InBurstCount);
}

USkeletalMeshComponent* AGP_MatadorBossDecoyActor::GetDecoyMesh() const
{
	return IsValid(DecoyMesh) ? DecoyMesh.Get() : GetMesh();
}

void AGP_MatadorBossDecoyActor::UpdateAnimationSet()
{
	ApplyAnimationSetToDecoyMesh();
	ApplyDefaultVisualLayout();
}

void AGP_MatadorBossDecoyActor::HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag)
{
	AGP_BaseCharacter::HandlePostDamageTaken(InstigatorActor, DamageAmount, ElementTag);
}

void AGP_MatadorBossDecoyActor::ApplyAnimationSetToDecoyMesh()
{
	USkeletalMeshComponent* MeshComponent = GetDecoyMesh();
	if (!IsValid(MeshComponent) || !IsValid(EnemyAnimationSet))
	{
		return;
	}

	if (IsValid(EnemyAnimationSet->CharacterMesh))
	{
		MeshComponent->SetSkeletalMeshAsset(EnemyAnimationSet->CharacterMesh);
	}

	if (EnemyAnimationSet->AnimBlueprintClass)
	{
		MeshComponent->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		MeshComponent->SetAnimInstanceClass(EnemyAnimationSet->AnimBlueprintClass);
	}
}

UGP_MatadorDecoyAnimInstance* AGP_MatadorBossDecoyActor::GetDecoyAnimInstance() const
{
	USkeletalMeshComponent* MeshComponent = GetDecoyMesh();
	return IsValid(MeshComponent) ? Cast<UGP_MatadorDecoyAnimInstance>(MeshComponent->GetAnimInstance()) : nullptr;
}
