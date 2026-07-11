#include "Game/Corruption/GP_CorruptionTestZoneActor.h"

#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_GameState.h"
#include "GameFramework/Pawn.h"
#include "UObject/ConstructorHelpers.h"

AGP_CorruptionTestZoneActor::AGP_CorruptionTestZoneActor()
{
	PrimaryActorTick.bCanEverTick = false;
	// Placed test stations must retain server authority semantics in multiplayer PIE.
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(SceneRoot);
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
	TriggerBox->SetCanEverAffectNavigation(false);

	VisualPlatform = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualPlatform"));
	VisualPlatform->SetupAttachment(SceneRoot);
	VisualPlatform->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualPlatform->SetGenerateOverlapEvents(false);
	VisualPlatform->SetCanEverAffectNavigation(false);
	VisualPlatform->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		VisualPlatform->SetStaticMesh(CylinderFinder.Object);
	}

	StationLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StationLabel"));
	StationLabel->SetupAttachment(SceneRoot);
	StationLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StationLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	StationLabel->SetWorldSize(72.0f);
	StationLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 170.0f));
	StationLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));

	StationLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StationLight"));
	StationLight->SetupAttachment(SceneRoot);
	StationLight->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	StationLight->SetIntensity(2500.0f);
	StationLight->SetAttenuationRadius(650.0f);
	StationLight->SetCastShadows(false);

	RefreshStationPresentation();
}

void AGP_CorruptionTestZoneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshStationPresentation();
}

void AGP_CorruptionTestZoneActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshStationPresentation();

	if (HasAuthority() && bApplyOnPlayerOverlap && TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerBeginOverlap);
	}

	if (HasAuthority() && bApplyOnBeginPlay)
	{
		ApplyTestCorruption();
	}
}

void AGP_CorruptionTestZoneActor::ConfigureWorldTestStation(float InCorruption, const FLinearColor& InStationColor)
{
	TestScope = EGPCorruptionTestScope::World;
	TestCorruption = FMath::Clamp(InCorruption, 0.0f, 100.0f);
	bApplyOnBeginPlay = false;
	bApplyOnPlayerOverlap = true;
	bPausePassiveIncreaseWhenApplied = true;
	StationColor = InStationColor;
	RefreshStationPresentation();
}

bool AGP_CorruptionTestZoneActor::ApplyTestCorruption()
{
	if (!HasAuthority())
	{
		return false;
	}

	UWorld* World = GetWorld();
	AGP_GameState* GameState = World ? World->GetGameState<AGP_GameState>() : nullptr;
	UGP_WorldCorruptionComponent* Corruption = IsValid(GameState) ? GameState->GetWorldCorruptionComponent() : nullptr;
	if (!IsValid(Corruption))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CorruptionTest] %s could not resolve GP_GameState corruption component."), *GetNameSafe(this));
		return false;
	}

	if (bPausePassiveIncreaseWhenApplied)
	{
		Corruption->SetPassiveIncreasePaused(true);
	}

	const float ClampedValue = FMath::Clamp(TestCorruption, 0.0f, Corruption->GetMaximumCorruption());
	const int32 AppliedRegionId = TestScope == EGPCorruptionTestScope::Region ? TestRegionId : INDEX_NONE;
	if (TestScope == EGPCorruptionTestScope::Region)
	{
		Corruption->SetRegionCorruption(TestRegionId, ClampedValue);
	}
	else
	{
		Corruption->SetWorldCorruption(ClampedValue);
	}

	UE_LOG(LogTemp, Display, TEXT("[CorruptionTest] Station=%s Scope=%s Region=%d Value=%.1f"),
		*GetNameSafe(this),
		TestScope == EGPCorruptionTestScope::World ? TEXT("World") : TEXT("Region"),
		AppliedRegionId,
		ClampedValue);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			3.0f,
			StationColor.ToFColor(true),
			FString::Printf(TEXT("CORRUPTION SET TO %.0f"), ClampedValue));
	}
	BP_OnTestCorruptionApplied(ClampedValue, AppliedRegionId);
	return true;
}

void AGP_CorruptionTestZoneActor::HandleTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const APawn* Pawn = Cast<APawn>(OtherActor);
	if (IsValid(Pawn) && Pawn->IsPlayerControlled())
	{
		ApplyTestCorruption();
	}
}

void AGP_CorruptionTestZoneActor::RefreshStationPresentation()
{
	const FVector SafeExtent(
		FMath::Max(100.0f, TriggerExtent.X),
		FMath::Max(100.0f, TriggerExtent.Y),
		FMath::Max(50.0f, TriggerExtent.Z));
	if (TriggerBox)
	{
		TriggerBox->SetBoxExtent(SafeExtent);
		TriggerBox->ShapeColor = StationColor.ToFColor(true);
	}
	if (VisualPlatform)
	{
		// Engine cylinder is 100cm wide/high; scale it into a thin, visible walk-in station.
		VisualPlatform->SetRelativeScale3D(FVector(SafeExtent.X / 50.0f, SafeExtent.Y / 50.0f, 0.08f));
	}
	if (StationLabel)
	{
		const FString ScopeLabel = TestScope == EGPCorruptionTestScope::World
			? TEXT("WORLD")
			: FString::Printf(TEXT("REGION %d"), TestRegionId);
		StationLabel->SetText(FText::FromString(FString::Printf(TEXT("%s CORRUPTION %.0f"), *ScopeLabel, TestCorruption)));
		StationLabel->SetTextRenderColor(StationColor.ToFColor(true));
	}
	if (StationLight)
	{
		StationLight->SetLightColor(StationColor);
	}
}
