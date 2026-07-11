#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AGP_RegionEventTestTriggerActor::AGP_RegionEventTestTriggerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TriggerSphere = CreateDefaultSubobject<USphereComponent>(TEXT("TriggerSphere"));
	TriggerSphere->SetupAttachment(SceneRoot);
	TriggerSphere->SetSphereRadius(TriggerRadius);
	TriggerSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerSphere->SetGenerateOverlapEvents(true);
	TriggerSphere->SetCanEverAffectNavigation(false);
	TriggerSphere->ShapeColor = FColor(255, 180, 40);

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
	StationLight->SetIntensity(3000.0f);
	StationLight->SetAttenuationRadius(900.0f);
	StationLight->SetCastShadows(false);

	FallbackEventActorClass = AGP_RegionEventActor::StaticClass();
	RefreshStationPresentation();
}

void AGP_RegionEventTestTriggerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshStationPresentation();
}

void AGP_RegionEventTestTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(FMath::Max(50.0f, TriggerRadius));
		if (HasAuthority() && bTriggerOnPlayerOverlap)
		{
			// The trigger is a testing convenience; production activation remains owned by RegionEventDirector.
			TriggerSphere->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleTriggerSphereOverlap);
		}
	}

	if (HasAuthority() && bTriggerOnBeginPlay)
	{
		if (UWorld* World = GetWorld())
		{
			// Delay one tick so placed actors and navmesh finish their normal BeginPlay setup before event enemies spawn.
			World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::TriggerRegionEventNextTick));
		}
	}
}

void AGP_RegionEventTestTriggerActor::ConfigureAsOverlapTestStation(int32 InRegionId)
{
	TestRegionId = FMath::Max(0, InRegionId);
	bTriggerOnBeginPlay = false;
	bTriggerOnPlayerOverlap = true;
	bAutoActivateSpawnedEvent = true;
	bShowStationVisuals = true;
	RefreshStationPresentation();
}

AGP_RegionEventActor* AGP_RegionEventTestTriggerActor::TriggerRegionEvent()
{
	if (!HasAuthority())
	{
		return nullptr;
	}

	if (IsValid(ActiveEvent)
		&& ActiveEvent->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
		&& ActiveEvent->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
	{
		return ActiveEvent;
	}

	if (!IsValid(EventData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[RegionEventTest] %s has no EventData assigned."), *GetNameSafe(this));
		return nullptr;
	}

	TSubclassOf<AGP_RegionEventActor> EventActorClass = EventData->EventActorClass
		? EventData->EventActorClass
		: FallbackEventActorClass;
	if (!*EventActorClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[RegionEventTest] %s cannot resolve an event actor class."), *GetNameSafe(this));
		return nullptr;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AGP_RegionEventActor* SpawnedEvent = World->SpawnActor<AGP_RegionEventActor>(
		EventActorClass,
		GetActorLocation() + EventSpawnOffset,
		GetActorRotation(),
		SpawnParameters);
	if (!IsValid(SpawnedEvent))
	{
		return nullptr;
	}

	ActiveEvent = SpawnedEvent;
	ActiveEvent->OnRegionEventStateChanged.AddDynamic(this, &ThisClass::HandleSpawnedEventStateChanged);
	ActiveEvent->InitializeRegionEvent(TestRegionId, EventData);
	if (bAutoActivateSpawnedEvent)
	{
		ActiveEvent->ActivateRegionEvent();
	}

	return ActiveEvent;
}

void AGP_RegionEventTestTriggerActor::CompleteActiveRegionEvent()
{
	if (HasAuthority() && IsValid(ActiveEvent))
	{
		// Lets designers stop long-running Red Rift/Crystal events from the Details panel while testing in PIE.
		ActiveEvent->CompleteRegionEvent();
	}
}

void AGP_RegionEventTestTriggerActor::TriggerRegionEventNextTick()
{
	// Timer delegates expect a void signature, while TriggerRegionEvent returns the spawned actor for editor utility calls.
	TriggerRegionEvent();
}

void AGP_RegionEventTestTriggerActor::HandleTriggerSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (Cast<AGP_PlayerCharacter>(OtherActor))
	{
		TriggerRegionEvent();
	}
}

void AGP_RegionEventTestTriggerActor::HandleSpawnedEventStateChanged(AGP_RegionEventActor* EventActor)
{
	if (EventActor == ActiveEvent
		&& IsValid(EventActor)
		&& (EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Completed
			|| EventActor->GetRuntimeState() == EGPRegionEventRuntimeState::Expired))
	{
		ActiveEvent = nullptr;
	}
}

void AGP_RegionEventTestTriggerActor::RefreshStationPresentation()
{
	const float SafeRadius = FMath::Max(50.0f, TriggerRadius);
	if (TriggerSphere)
	{
		TriggerSphere->SetSphereRadius(SafeRadius);
		TriggerSphere->ShapeColor = StationColor.ToFColor(true);
	}
	if (VisualPlatform)
	{
		// Engine cylinder is 100cm wide/high; keep the test station thin and easy to walk across.
		VisualPlatform->SetRelativeScale3D(FVector(SafeRadius / 50.0f, SafeRadius / 50.0f, 0.08f));
		VisualPlatform->SetVisibility(bShowStationVisuals, true);
	}
	if (StationLabel)
	{
		StationLabel->SetText(StationTitle);
		StationLabel->SetTextRenderColor(StationColor.ToFColor(true));
		StationLabel->SetVisibility(bShowStationVisuals, true);
	}
	if (StationLight)
	{
		StationLight->SetLightColor(StationColor);
		StationLight->SetVisibility(bShowStationVisuals, true);
	}
}
