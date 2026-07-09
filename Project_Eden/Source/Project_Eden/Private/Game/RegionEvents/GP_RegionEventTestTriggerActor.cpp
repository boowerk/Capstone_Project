#include "Game/RegionEvents/GP_RegionEventTestTriggerActor.h"

#include "Characters/GP_PlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "TimerManager.h"

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
	TriggerSphere->ShapeColor = FColor(255, 180, 40);

	FallbackEventActorClass = AGP_RegionEventActor::StaticClass();
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
