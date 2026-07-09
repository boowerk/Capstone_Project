#include "Actors/GP_BossDeathPresentationActor.h"

#include "Animation/AnimationAsset.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

AGP_BossDeathPresentationActor::AGP_BossDeathPresentationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		CubeMesh = CubeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMeshFinder(TEXT("/Engine/BasicShapes/Cone.Cone"));
	if (ConeMeshFinder.Succeeded())
	{
		ConeMesh = ConeMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		CylinderMesh = CylinderMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HandMeshFinder(
		TEXT("/Game/Meshes/PLAZA_DE_TOROS/ActorMesh/SK_RightHand.SK_RightHand"));
	if (HandMeshFinder.Succeeded())
	{
		// The gameplay hitbox remains elsewhere; this skeletal mesh is only a readable death presentation prop.
		SansHandMesh = HandMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BullMeshFinder(TEXT("/Game/Meshes/Bull/SK/Bull_High.Bull_High"));
	if (BullMeshFinder.Succeeded())
	{
		BullMesh = BullMeshFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAnimationAsset> BullRunFinder(TEXT("/Game/Meshes/Bull/SK/Bull_run_ScaleFix100.Bull_run_ScaleFix100"));
	if (BullRunFinder.Succeeded())
	{
		BullRunAnimation = BullRunFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> BasicMaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (BasicMaterialFinder.Succeeded())
	{
		BasicShapeMaterial = BasicMaterialFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DarkLightningFinder(
		TEXT("/Game/Niagara/Vefects/Easy_Impact_Frames/VFX/Extras/Particles/NS_Extra_Lightning_Example_VFX.NS_Extra_Lightning_Example_VFX"));
	if (DarkLightningFinder.Succeeded())
	{
		DefaultDarkLightningSystem = DarkLightningFinder.Object;
	}
}

void AGP_BossDeathPresentationActor::InitializePresentation(
	EGPBossDeathPresentationStyle InPresentationStyle,
	AActor* InSourceBoss,
	AActor* InInstigatorActor,
	const FGPBossDeathPresentationSpawnSettings& InSettings)
{
	PresentationStyle = InPresentationStyle;
	SourceBossActor = InSourceBoss;
	PresentationInstigatorActor = InInstigatorActor;
	Settings = InSettings;
	ElapsedSeconds = 0.0f;

	if (IsValid(InSourceBoss))
	{
		SetActorLocation(InSourceBoss->GetActorLocation());
		SetActorRotation(InSourceBoss->GetActorRotation());
	}

	// Location and style seed the pieces so repeated kills feel consistent but not perfectly symmetric.
	RandomStream.Initialize(HashCombineFast(GetTypeHash(GetActorLocation()), static_cast<uint32>(PresentationStyle)));
	SetLifeSpan(FMath::Max(0.1f, Settings.LifeSpanSeconds));
	StartPresentation();
}

void AGP_BossDeathPresentationActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	ElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	for (FPieceMotion& Motion : PieceMotions)
	{
		USceneComponent* Piece = Motion.Component.Get();
		if (!IsValid(Piece))
		{
			continue;
		}

		const bool bShouldStart = ElapsedSeconds >= Motion.StartDelaySeconds;
		if (!bShouldStart)
		{
			Piece->SetVisibility(false, true);
			continue;
		}

		if (!Motion.bStarted)
		{
			Motion.bStarted = true;
			Piece->SetVisibility(true, true);
		}

		if (ElapsedSeconds >= Motion.HideAtSeconds)
		{
			Piece->SetVisibility(false, true);
			continue;
		}

		Piece->AddWorldOffset(Motion.Velocity * DeltaSeconds, false);
		Piece->AddWorldRotation(FRotator(
			Motion.AngularVelocity.Pitch * DeltaSeconds,
			Motion.AngularVelocity.Yaw * DeltaSeconds,
			Motion.AngularVelocity.Roll * DeltaSeconds));
		Motion.Velocity.Z -= 980.0f * Motion.GravityScale * DeltaSeconds;

		if (Motion.bShrinkOverLifetime)
		{
			const float Lifetime = FMath::Max(0.01f, Motion.HideAtSeconds - Motion.StartDelaySeconds);
			const float Alpha = FMath::Clamp((ElapsedSeconds - Motion.StartDelaySeconds) / Lifetime, 0.0f, 1.0f);
			Piece->SetWorldScale3D(FMath::Lerp(Motion.InitialScale, Motion.TargetScale, Alpha));
		}
	}
}

int32 AGP_BossDeathPresentationActor::GetSpawnedPieceCount() const
{
	return StaticPieces.Num() + SkeletalPieces.Num();
}

void AGP_BossDeathPresentationActor::StartPresentation()
{
	if (Settings.bHideSourceMesh)
	{
		if (Settings.SourceMeshHideDelaySeconds <= KINDA_SMALL_NUMBER)
		{
			HideSourceMesh();
		}
		else if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				SourceMeshHideTimerHandle,
				this,
				&ThisClass::HideSourceMesh,
				Settings.SourceMeshHideDelaySeconds,
				false);
		}
	}

	switch (PresentationStyle)
	{
	case EGPBossDeathPresentationStyle::CrystalSeraph:
		StartCrystalSeraphPresentation();
		break;
	case EGPBossDeathPresentationStyle::Sans:
		StartSansPresentation();
		break;
	case EGPBossDeathPresentationStyle::DarkArmorKnight:
		StartDarkArmorKnightPresentation();
		break;
	case EGPBossDeathPresentationStyle::Matador:
		StartMatadorPresentation();
		break;
	default:
		break;
	}

	BP_OnPresentationStarted(PresentationStyle, SourceBossActor.Get(), PresentationInstigatorActor.Get());
}

void AGP_BossDeathPresentationActor::StartCrystalSeraphPresentation()
{
	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, GetScaled(180.0f));
	const FLinearColor CrystalTint(0.34f, 0.68f, 1.0f, 1.0f);
	SpawnBurstNiagara(Settings.OverrideBurstNiagara, FVector(0.0f, 0.0f, GetScaled(120.0f)), FVector(GetScaled(1.25f)));

	for (int32 Index = 0; Index < 38; ++Index)
	{
		const FVector Direction = RandomHorizontalDirection();
		const FVector Location = Center
			+ Direction * RandomStream.FRandRange(GetScaled(20.0f), GetScaled(140.0f))
			+ FVector(0.0f, 0.0f, RandomStream.FRandRange(GetScaled(-40.0f), GetScaled(120.0f)));
		const FVector Velocity = Direction * RandomStream.FRandRange(GetScaled(160.0f), GetScaled(520.0f))
			+ FVector(0.0f, 0.0f, RandomStream.FRandRange(GetScaled(120.0f), GetScaled(500.0f)));

		AddStaticPiece(
			ConeMesh ? ConeMesh.Get() : CubeMesh.Get(),
			Location,
			FRotator(RandomStream.FRandRange(-60.0f, 60.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-35.0f, 35.0f)),
			FVector(
				RandomStream.FRandRange(GetScaled(0.18f), GetScaled(0.35f)),
				RandomStream.FRandRange(GetScaled(0.18f), GetScaled(0.32f)),
				RandomStream.FRandRange(GetScaled(0.55f), GetScaled(1.15f))),
			Velocity,
			FRotator(RandomStream.FRandRange(-160.0f, 160.0f), RandomStream.FRandRange(-220.0f, 220.0f), RandomStream.FRandRange(-180.0f, 180.0f)),
			1.35f,
			4.2f,
			CrystalTint);
	}

	for (int32 Index = 0; Index < 14; ++Index)
	{
		const FVector Location = Center
			+ FVector(RandomStream.FRandRange(GetScaled(-650.0f), GetScaled(650.0f)), RandomStream.FRandRange(GetScaled(-650.0f), GetScaled(650.0f)), GetScaled(460.0f));
		AddStaticPiece(
			ConeMesh ? ConeMesh.Get() : CubeMesh.Get(),
			Location,
			FRotator(-90.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f),
			FVector(GetScaled(0.12f), GetScaled(0.12f), GetScaled(0.8f)),
			FVector(0.0f, 0.0f, RandomStream.FRandRange(GetScaled(-650.0f), GetScaled(-380.0f))),
			FRotator(0.0f, RandomStream.FRandRange(-90.0f, 90.0f), RandomStream.FRandRange(-90.0f, 90.0f)),
			0.35f,
			4.6f,
			CrystalTint,
			RandomStream.FRandRange(0.15f, 0.85f));
	}
}

void AGP_BossDeathPresentationActor::StartSansPresentation()
{
	const FVector GroundCenter = ResolveGroundLocation(GetActorLocation());
	const FLinearColor CrackTint(0.02f, 0.0f, 0.04f, 1.0f);
	const FLinearColor HandTint(0.18f, 0.18f, 0.2f, 1.0f);

	for (int32 Index = 0; Index < 5; ++Index)
	{
		const float Angle = 72.0f * Index + RandomStream.FRandRange(-14.0f, 14.0f);
		const FVector Direction = FRotator(0.0f, Angle, 0.0f).Vector();
		const FVector Location = ResolveGroundLocation(GroundCenter + Direction * GetScaled(RandomStream.FRandRange(140.0f, 360.0f)));
		AddSkeletalPiece(
			SansHandMesh.Get(),
			nullptr,
			Location + FVector(0.0f, 0.0f, GetScaled(65.0f)),
			(Direction * -1.0f).Rotation() + FRotator(0.0f, 0.0f, RandomStream.FRandRange(-16.0f, 16.0f)),
			FVector(GetScaled(RandomStream.FRandRange(0.55f, 0.78f))),
			FVector(0.0f, 0.0f, GetScaled(-150.0f)),
			FRotator(0.0f, RandomStream.FRandRange(-35.0f, 35.0f), 0.0f),
			0.0f,
			2.4f,
			HandTint,
			Index * 0.12f,
			true,
			FVector(GetScaled(0.08f)));
	}

	for (int32 Index = 0; Index < 7; ++Index)
	{
		const float Angle = Index * 25.0f + RandomStream.FRandRange(-8.0f, 8.0f);
		AddStaticPiece(
			CubeMesh.Get(),
			GroundCenter + FVector(0.0f, 0.0f, GetScaled(4.0f)),
			FRotator(0.0f, Angle, 0.0f),
			FVector(GetScaled(RandomStream.FRandRange(4.2f, 7.0f)), GetScaled(0.12f), GetScaled(0.018f)),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			0.0f,
			2.8f,
			CrackTint,
			0.0f,
			true,
			FVector(GetScaled(0.08f), GetScaled(0.02f), GetScaled(0.01f)));
	}
}

void AGP_BossDeathPresentationActor::StartDarkArmorKnightPresentation()
{
	const FVector Center = GetActorLocation() + FVector(0.0f, 0.0f, GetScaled(110.0f));
	const FLinearColor ArmorTint(0.03f, 0.025f, 0.04f, 1.0f);
	const FLinearColor SparkTint(0.1f, 0.02f, 0.18f, 1.0f);
	SpawnBurstNiagara(Settings.OverrideBurstNiagara ? Settings.OverrideBurstNiagara.Get() : DefaultDarkLightningSystem.Get(), FVector::ZeroVector, FVector(GetScaled(1.6f)));

	for (int32 Index = 0; Index < 24; ++Index)
	{
		const FVector Direction = RandomHorizontalDirection();
		AddStaticPiece(
			CubeMesh.Get(),
			Center + Direction * RandomStream.FRandRange(GetScaled(25.0f), GetScaled(90.0f)),
			FRotator(RandomStream.FRandRange(-45.0f, 45.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-45.0f, 45.0f)),
			FVector(
				RandomStream.FRandRange(GetScaled(0.16f), GetScaled(0.45f)),
				RandomStream.FRandRange(GetScaled(0.06f), GetScaled(0.18f)),
				RandomStream.FRandRange(GetScaled(0.12f), GetScaled(0.36f))),
			Direction * RandomStream.FRandRange(GetScaled(220.0f), GetScaled(620.0f)) + FVector(0.0f, 0.0f, RandomStream.FRandRange(GetScaled(160.0f), GetScaled(520.0f))),
			FRotator(RandomStream.FRandRange(-260.0f, 260.0f), RandomStream.FRandRange(-320.0f, 320.0f), RandomStream.FRandRange(-280.0f, 280.0f)),
			1.1f,
			3.6f,
			Index % 5 == 0 ? SparkTint : ArmorTint);
	}
}

void AGP_BossDeathPresentationActor::StartMatadorPresentation()
{
	const FVector GroundCenter = ResolveGroundLocation(GetActorLocation());
	const FVector Forward = GetActorForwardVector().GetSafeNormal2D();
	const FVector BullDirection = Forward.IsNearlyZero() ? FVector::ForwardVector : Forward;
	const FLinearColor BullTint(1.0f, 0.08f, 0.04f, 1.0f);
	const FLinearColor StoneTint(0.42f, 0.32f, 0.24f, 1.0f);
	SpawnBurstNiagara(Settings.OverrideBurstNiagara, FVector(0.0f, 0.0f, GetScaled(60.0f)), FVector(GetScaled(1.15f)));

	AddSkeletalPiece(
		BullMesh.Get(),
		BullRunAnimation.Get(),
		GroundCenter + BullDirection * GetScaled(120.0f) + FVector(0.0f, 0.0f, GetScaled(55.0f)),
		BullDirection.Rotation(),
		FVector(GetScaled(0.75f)),
		BullDirection * GetScaled(780.0f) + FVector(0.0f, 0.0f, GetScaled(65.0f)),
		FRotator(0.0f, 0.0f, 0.0f),
		0.0f,
		1.55f,
		BullTint,
		0.0f,
		true,
		FVector(GetScaled(0.2f)));

	for (int32 Index = 0; Index < 32; ++Index)
	{
		const float Radius = RandomStream.FRandRange(GetScaled(430.0f), GetScaled(1250.0f));
		const FVector Direction = RandomHorizontalDirection();
		const FVector Location = ResolveGroundLocation(GroundCenter + Direction * Radius) + FVector(0.0f, 0.0f, GetScaled(16.0f));
		AddStaticPiece(
			Index % 3 == 0 && CylinderMesh ? CylinderMesh.Get() : CubeMesh.Get(),
			Location,
			FRotator(RandomStream.FRandRange(-12.0f, 12.0f), RandomStream.FRandRange(0.0f, 360.0f), RandomStream.FRandRange(-8.0f, 8.0f)),
			FVector(
				RandomStream.FRandRange(GetScaled(0.18f), GetScaled(0.55f)),
				RandomStream.FRandRange(GetScaled(0.18f), GetScaled(0.55f)),
				RandomStream.FRandRange(GetScaled(0.05f), GetScaled(0.16f))),
			Direction * RandomStream.FRandRange(GetScaled(25.0f), GetScaled(180.0f)) + FVector(0.0f, 0.0f, RandomStream.FRandRange(GetScaled(80.0f), GetScaled(260.0f))),
			FRotator(RandomStream.FRandRange(-90.0f, 90.0f), RandomStream.FRandRange(-160.0f, 160.0f), RandomStream.FRandRange(-120.0f, 120.0f)),
			0.95f,
			3.4f,
			StoneTint,
			RandomStream.FRandRange(0.0f, 0.45f));
	}
}

void AGP_BossDeathPresentationActor::SpawnBurstNiagara(UNiagaraSystem* NiagaraSystem, const FVector& Offset, const FVector& Scale)
{
	if (!IsValid(NiagaraSystem))
	{
		return;
	}

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		NiagaraSystem,
		GetActorLocation() + Offset,
		GetActorRotation(),
		Scale,
		true,
		true,
		ENCPoolMethod::AutoRelease);
}

void AGP_BossDeathPresentationActor::HideSourceMesh()
{
	AActor* Boss = SourceBossActor.Get();
	if (!IsValid(Boss))
	{
		return;
	}

	TArray<USkeletalMeshComponent*> MeshComponents;
	Boss->GetComponents(MeshComponents);
	for (USkeletalMeshComponent* MeshComponent : MeshComponents)
	{
		if (IsValid(MeshComponent))
		{
			// Hide only the dying boss' visuals; gameplay collision has already been disabled by enemy death state.
			MeshComponent->SetHiddenInGame(true, true);
		}
	}
}

void AGP_BossDeathPresentationActor::AddStaticPiece(
	UStaticMesh* Mesh,
	const FVector& WorldLocation,
	const FRotator& WorldRotation,
	const FVector& WorldScale,
	const FVector& Velocity,
	const FRotator& AngularVelocity,
	float GravityScale,
	float HideAtSeconds,
	const FLinearColor& Tint,
	float StartDelaySeconds,
	bool bShrinkOverLifetime,
	const FVector& TargetScale)
{
	if (!IsValid(Mesh))
	{
		return;
	}

	const FName ComponentName = MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("DeathStaticPiece"));
	UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this, ComponentName);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece->SetGenerateOverlapEvents(false);
	Piece->SetCastShadow(false);
	Piece->SetStaticMesh(Mesh);
	if (UMaterialInterface* TintMaterial = CreateTintMaterial(Tint))
	{
		Piece->SetMaterial(0, TintMaterial);
	}
	Piece->SetupAttachment(SceneRoot);
	Piece->RegisterComponent();
	Piece->SetWorldLocationAndRotation(WorldLocation, WorldRotation);
	Piece->SetWorldScale3D(WorldScale);
	Piece->SetVisibility(StartDelaySeconds <= KINDA_SMALL_NUMBER, true);
	StaticPieces.Add(Piece);

	FPieceMotion Motion;
	Motion.Component = Piece;
	Motion.Velocity = Velocity;
	Motion.AngularVelocity = AngularVelocity;
	Motion.GravityScale = FMath::Max(0.0f, GravityScale);
	Motion.StartDelaySeconds = FMath::Max(0.0f, StartDelaySeconds);
	Motion.HideAtSeconds = FMath::Max(Motion.StartDelaySeconds + 0.01f, HideAtSeconds);
	Motion.InitialScale = WorldScale;
	Motion.TargetScale = TargetScale.IsNearlyZero() ? FVector::ZeroVector : TargetScale;
	Motion.bShrinkOverLifetime = bShrinkOverLifetime;
	Motion.bStarted = StartDelaySeconds <= KINDA_SMALL_NUMBER;
	PieceMotions.Add(Motion);
}

void AGP_BossDeathPresentationActor::AddSkeletalPiece(
	USkeletalMesh* Mesh,
	UAnimationAsset* Animation,
	const FVector& WorldLocation,
	const FRotator& WorldRotation,
	const FVector& WorldScale,
	const FVector& Velocity,
	const FRotator& AngularVelocity,
	float GravityScale,
	float HideAtSeconds,
	const FLinearColor& Tint,
	float StartDelaySeconds,
	bool bShrinkOverLifetime,
	const FVector& TargetScale)
{
	if (!IsValid(Mesh))
	{
		return;
	}

	const FName ComponentName = MakeUniqueObjectName(this, USkeletalMeshComponent::StaticClass(), TEXT("DeathSkeletalPiece"));
	USkeletalMeshComponent* Piece = NewObject<USkeletalMeshComponent>(this, ComponentName);
	Piece->SetMobility(EComponentMobility::Movable);
	Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Piece->SetGenerateOverlapEvents(false);
	Piece->SetCastShadow(false);
	Piece->SetSkeletalMeshAsset(Mesh);
	if (IsValid(Animation))
	{
		Piece->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		Piece->SetAnimation(Animation);
		Piece->Play(true);
	}
	if (UMaterialInterface* TintMaterial = CreateTintMaterial(Tint))
	{
		Piece->SetMaterial(0, TintMaterial);
	}
	Piece->SetupAttachment(SceneRoot);
	Piece->RegisterComponent();
	Piece->SetWorldLocationAndRotation(WorldLocation, WorldRotation);
	Piece->SetWorldScale3D(WorldScale);
	Piece->SetVisibility(StartDelaySeconds <= KINDA_SMALL_NUMBER, true);
	SkeletalPieces.Add(Piece);

	FPieceMotion Motion;
	Motion.Component = Piece;
	Motion.Velocity = Velocity;
	Motion.AngularVelocity = AngularVelocity;
	Motion.GravityScale = FMath::Max(0.0f, GravityScale);
	Motion.StartDelaySeconds = FMath::Max(0.0f, StartDelaySeconds);
	Motion.HideAtSeconds = FMath::Max(Motion.StartDelaySeconds + 0.01f, HideAtSeconds);
	Motion.InitialScale = WorldScale;
	Motion.TargetScale = TargetScale.IsNearlyZero() ? FVector::ZeroVector : TargetScale;
	Motion.bShrinkOverLifetime = bShrinkOverLifetime;
	Motion.bStarted = StartDelaySeconds <= KINDA_SMALL_NUMBER;
	PieceMotions.Add(Motion);
}

UMaterialInterface* AGP_BossDeathPresentationActor::CreateTintMaterial(const FLinearColor& Tint)
{
	if (!IsValid(BasicShapeMaterial))
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(BasicShapeMaterial, this);
	if (!IsValid(DynamicMaterial))
	{
		return nullptr;
	}

	// Different placeholder materials expose different parameter names, so set the common options defensively.
	DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
	DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Tint);
	DynamicMaterial->SetVectorParameterValue(TEXT("Tint"), Tint);
	DynamicMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), Tint);
	DynamicMaterials.Add(DynamicMaterial);
	return DynamicMaterial;
}

FVector AGP_BossDeathPresentationActor::ResolveGroundLocation(const FVector& DesiredLocation) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return DesiredLocation;
	}

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossDeathPresentationGroundTrace), false, SourceBossActor.Get());
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	if (World->LineTraceSingleByObjectType(
		GroundHit,
		DesiredLocation + FVector(0.0f, 0.0f, 500.0f),
		DesiredLocation - FVector(0.0f, 0.0f, 2500.0f),
		ObjectQueryParams,
		QueryParams))
	{
		return GroundHit.ImpactPoint;
	}

	return DesiredLocation;
}

FVector AGP_BossDeathPresentationActor::RandomHorizontalDirection()
{
	const float AngleDegrees = RandomStream.FRandRange(0.0f, 360.0f);
	return FRotator(0.0f, AngleDegrees, 0.0f).Vector();
}

float AGP_BossDeathPresentationActor::GetScaled(float Value) const
{
	return Value * FMath::Max(0.01f, Settings.UniformScale);
}
