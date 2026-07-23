#include "Actors/GP_BossGroundHandActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "GameplayTags/GP_Tags.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "Utils/GP_BlueprintLibrary.h"
#include "UObject/ConstructorHelpers.h"

AGP_BossGroundHandActor::AGP_BossGroundHandActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	HandVisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("HandVisualRoot"));
	HandVisualRoot->SetupAttachment(SceneRoot);

	WarningDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("WarningDecal"));
	WarningDecal->SetupAttachment(SceneRoot);
	WarningDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	WarningDecal->DecalColor = FLinearColor(1.0f, 0.0f, 0.0f, 0.62f);
	WarningDecal->SetFadeScreenSize(0.001f);
	WarningDecal->SetSortOrder(8);

	HandCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HandCollision"));
	HandCollision->SetupAttachment(HandVisualRoot);
	HandCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	HandCollision->SetBoxExtent(FVector(55.0f, 62.0f, 110.0f));
	HandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HandCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HandCollision->SetGenerateOverlapEvents(true);

	// Use the imported right-hand skeletal asset in its reference pose; gameplay collision remains on HandCollision.
	HandMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("HandMesh"));
	HandMesh->SetupAttachment(HandVisualRoot);
	HandMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandMesh->SetGenerateOverlapEvents(false);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandMeshFinder(
		TEXT("/Game/Meshes/PLAZA_DE_TOROS/ActorMesh/SK_RightHand.SK_RightHand"));
	if (HandMeshFinder.Succeeded())
	{
		HandMesh->SetSkeletalMeshAsset(HandMeshFinder.Object);
	}

	// A dedicated radial opacity mask keeps the square projector from appearing as the attack warning.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMaterialFinder(
		TEXT("/Game/Effects/M_BossGroundHandTelegraph_Decal.M_BossGroundHandTelegraph_Decal"));
	if (DecalMaterialFinder.Succeeded())
	{
		WarningDecalMaterial = DecalMaterialFinder.Object;
		WarningDecal->SetDecalMaterial(WarningDecalMaterial);
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(
		TEXT("/Game/GAS_Pattern/AbilitySystem/GameplayEffects/Damage/GE_PrimaryDamage"));
	if (DamageEffectFinder.Succeeded())
	{
		DamageEffectClass = DamageEffectFinder.Class;
	}
}

void AGP_BossGroundHandActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Blueprint children tune only the skeletal presentation; HandCollision remains an unchanged sibling component.
	ApplyHandVisualSettings();
}

void AGP_BossGroundHandActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyHandVisualSettings();
	// Runtime starts with only the warning decal visible; the BP viewport remains available for mesh tuning.
	HandMesh->SetHiddenInGame(true, true);
	HandCollision->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnHandOverlap);
	WarningDecal->DecalSize = FVector(120.0f, WarningRadius, WarningRadius);
	if (WarningDecalMaterial)
	{
		WarningDecal->SetDecalMaterial(WarningDecalMaterial);
	}

	HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HiddenDepth));
	WarningDecal->SetVisibility(false);
	if (PatternSpec.bInitialized)
	{
		ApplyPatternStart();
	}
}

void AGP_BossGroundHandActor::ApplyHandVisualSettings()
{
	if (!IsValid(HandMesh))
	{
		return;
	}

	// Uniform scaling avoids distorting the imported hand while offset and rotation stay available for art alignment.
	HandMesh->SetRelativeScale3D(FVector(FMath::Max(0.01f, HandVisualScale)));
	HandMesh->SetRelativeLocation(HandVisualOffset);
	HandMesh->SetRelativeRotation(HandVisualRotation);
}

void AGP_BossGroundHandActor::InitializeGroundHand(float InTelegraphDuration)
{
	if (!HasAuthority())
	{
		return;
	}

	SnapToFloor();
	PatternSpec.TelegraphDuration = FMath::Max(0.0f, InTelegraphDuration);
	PatternSpec.RiseDuration = FMath::Max(0.01f, RiseDuration);
	PatternSpec.HoldDuration = FMath::Max(0.0f, HoldDuration);
	PatternSpec.RetractDuration = FMath::Max(0.01f, RetractDuration);
	PatternSpec.bInitialized = true;
	ApplyPatternStart();
	ForceNetUpdate();

	const float TotalDuration = PatternSpec.TelegraphDuration + PatternSpec.RiseDuration
		+ PatternSpec.HoldDuration + PatternSpec.RetractDuration;
	SetLifeSpan(TotalDuration + 0.25f);
}

void AGP_BossGroundHandActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bPatternStarted)
	{
		return;
	}

	LocalElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	UpdatePatternVisuals(LocalElapsedSeconds);
}

void AGP_BossGroundHandActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_BossGroundHandActor, PatternSpec);
}

void AGP_BossGroundHandActor::OnRep_PatternSpec()
{
	ApplyPatternStart();
}

void AGP_BossGroundHandActor::ApplyPatternStart()
{
	if (!PatternSpec.bInitialized)
	{
		return;
	}

	LocalElapsedSeconds = 0.0f;
	bPatternStarted = true;
	bCollisionActive = false;
	HitActors.Reset();
	HandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HiddenDepth));
	HandMesh->SetHiddenInGame(true, true);
	WarningDecal->SetVisibility(true);
}

void AGP_BossGroundHandActor::UpdatePatternVisuals(float ElapsedSeconds)
{
	const float TelegraphEnd = PatternSpec.TelegraphDuration;
	const float RiseEnd = TelegraphEnd + PatternSpec.RiseDuration;
	const float HoldEnd = RiseEnd + PatternSpec.HoldDuration;
	const float RetractEnd = HoldEnd + PatternSpec.RetractDuration;

	if (ElapsedSeconds < TelegraphEnd)
	{
		WarningDecal->SetVisibility(true);
		HandMesh->SetHiddenInGame(true, true);
		HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HiddenDepth));
		return;
	}

	WarningDecal->SetVisibility(false);
	// Reveal on the first rise frame so no part of a large skeletal mesh leaks through during the warning.
	HandMesh->SetHiddenInGame(false, true);
	if (!bCollisionActive)
	{
		bCollisionActive = true;
		HandCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}

	float VisualHeight = 0.0f;
	if (ElapsedSeconds < RiseEnd)
	{
		const float RiseAlpha = FMath::Clamp((ElapsedSeconds - TelegraphEnd) / PatternSpec.RiseDuration, 0.0f, 1.0f);
		VisualHeight = FMath::Lerp(-HiddenDepth, 0.0f, FMath::InterpEaseOut(0.0f, 1.0f, RiseAlpha, 2.0f));
	}
	else if (ElapsedSeconds < HoldEnd)
	{
		VisualHeight = 0.0f;
	}
	else if (ElapsedSeconds < RetractEnd)
	{
		const float RetractAlpha = FMath::Clamp((ElapsedSeconds - HoldEnd) / PatternSpec.RetractDuration, 0.0f, 1.0f);
		VisualHeight = FMath::Lerp(0.0f, -HiddenDepth, RetractAlpha);
	}
	else
	{
		HandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		HandMesh->SetHiddenInGame(true, true);
		HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HiddenDepth));
		bPatternStarted = false;
		return;
	}

	HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, VisualHeight));
}

void AGP_BossGroundHandActor::OnHandOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || !bCollisionActive || !IsValid(OtherActor) || HitActors.Contains(OtherActor)
		|| !UGP_BlueprintLibrary::CanApplyCombatEffect(GetInstigator(), OtherActor))
	{
		return;
	}

	HitActors.Add(OtherActor);
	ApplyDamageAndLaunch(OtherActor);
}

void AGP_BossGroundHandActor::ApplyDamageAndLaunch(AActor* TargetActor)
{
	UAbilitySystemComponent* SourceASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetInstigator());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (IsValid(SourceASC) && IsValid(TargetASC) && DamageEffectClass)
	{
		FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
		ContextHandle.AddInstigator(GetInstigator(), this);
		FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			// Native prototype damage does not depend on a SkillData asset, so provide the shared execution values explicitly.
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Base, BaseDamage);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::BaseSpell, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::ToughnessBase, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Data::Multiplier, 1.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Atk, AttackPowerDamageCoefficient);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::M_Atk, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Def, 0.0f);
			SpecHandle.Data->SetSetByCallerMagnitude(GPTags::Damage::Coef::Hp, 0.0f);
			SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
		}
	}

	TArray<AActor*> HitActorArray{TargetActor};
	UGP_BlueprintLibrary::SendGameplayEventToActors(GetInstigator(), HitActorArray, GPTags::Event::Player::HitReact);

	// LaunchCharacter hands vertical velocity to CharacterMovement, which naturally transitions into falling under gravity.
	if (ACharacter* HitCharacter = Cast<ACharacter>(TargetActor))
	{
		HitCharacter->LaunchCharacter(FVector(0.0f, 0.0f, LaunchSpeed), false, true);
	}
	else if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		RootPrimitive->AddImpulse(FVector(0.0f, 0.0f, LaunchSpeed * RootPrimitive->GetMass()), NAME_None, true);
	}
}

void AGP_BossGroundHandActor::SnapToFloor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	FHitResult FloorHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossGroundHandFloorTrace), false, GetOwner());
	FCollisionObjectQueryParams FloorObjectQuery;
	// The center strike overlaps the target pawn, so only static level geometry may establish the decal floor.
	FloorObjectQuery.AddObjectTypesToQuery(ECC_WorldStatic);
	if (World->LineTraceSingleByObjectType(
		FloorHit,
		CurrentLocation + FVector(0.0f, 0.0f, FloorTraceUpDistance),
		CurrentLocation - FVector(0.0f, 0.0f, FloorTraceDownDistance),
		FloorObjectQuery,
		QueryParams))
	{
		SetActorLocation(FloorHit.ImpactPoint + FVector(0.0f, 0.0f, 3.0f));
	}
}
