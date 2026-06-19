#include "Actors/GP_BossGroundHandActor.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
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

	HandCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("HandCollision"));
	HandCollision->SetupAttachment(HandVisualRoot);
	HandCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	HandCollision->SetBoxExtent(FVector(55.0f, 62.0f, 110.0f));
	HandCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HandCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	HandCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	HandCollision->SetGenerateOverlapEvents(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object.Get() : nullptr;
	UStaticMesh* CylinderMesh = CylinderMeshFinder.Succeeded() ? CylinderMeshFinder.Object.Get() : CubeMesh;

	// Assemble a readable placeholder hand from engine primitives until the final hand asset is available.
	PalmMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PalmMesh"));
	PalmMesh->SetupAttachment(HandVisualRoot);
	PalmMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 35.0f));
	PalmMesh->SetRelativeScale3D(FVector(0.7f, 0.5f, 0.8f));
	ConfigureMeshComponent(PalmMesh, CubeMesh);

	WristMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WristMesh"));
	WristMesh->SetupAttachment(HandVisualRoot);
	WristMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -55.0f));
	WristMesh->SetRelativeScale3D(FVector(0.28f, 0.28f, 1.0f));
	ConfigureMeshComponent(WristMesh, CylinderMesh);

	const float FingerYLocations[] = {-30.0f, -10.0f, 10.0f, 30.0f};
	const float FingerLengthScales[] = {0.58f, 0.72f, 0.68f, 0.52f};
	for (int32 FingerIndex = 0; FingerIndex < 4; ++FingerIndex)
	{
		UStaticMeshComponent* FingerMesh = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("FingerMesh%d"), FingerIndex));
		FingerMesh->SetupAttachment(HandVisualRoot);
		FingerMesh->SetRelativeLocation(FVector(0.0f, FingerYLocations[FingerIndex], 112.0f));
		FingerMesh->SetRelativeScale3D(FVector(0.11f, 0.11f, FingerLengthScales[FingerIndex]));
		ConfigureMeshComponent(FingerMesh, CylinderMesh);
		FingerMeshes.Add(FingerMesh);
	}

	UStaticMeshComponent* ThumbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ThumbMesh"));
	ThumbMesh->SetupAttachment(HandVisualRoot);
	ThumbMesh->SetRelativeLocation(FVector(0.0f, -55.0f, 48.0f));
	ThumbMesh->SetRelativeRotation(FRotator(0.0f, 0.0f, 48.0f));
	ThumbMesh->SetRelativeScale3D(FVector(0.13f, 0.13f, 0.5f));
	ConfigureMeshComponent(ThumbMesh, CylinderMesh);
	FingerMeshes.Add(ThumbMesh);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DecalMaterialFinder(
		TEXT("/Engine/EngineMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));
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

void AGP_BossGroundHandActor::BeginPlay()
{
	Super::BeginPlay();

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
		HandVisualRoot->SetRelativeLocation(FVector(0.0f, 0.0f, -HiddenDepth));
		return;
	}

	WarningDecal->SetVisibility(false);
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
	if (World->LineTraceSingleByChannel(
		FloorHit,
		CurrentLocation + FVector(0.0f, 0.0f, FloorTraceUpDistance),
		CurrentLocation - FVector(0.0f, 0.0f, FloorTraceDownDistance),
		ECC_Visibility,
		QueryParams))
	{
		SetActorLocation(FloorHit.ImpactPoint + FVector(0.0f, 0.0f, 3.0f));
	}
}

void AGP_BossGroundHandActor::ConfigureMeshComponent(UStaticMeshComponent* MeshComponent, UStaticMesh* MeshAsset) const
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetStaticMesh(MeshAsset);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
}
