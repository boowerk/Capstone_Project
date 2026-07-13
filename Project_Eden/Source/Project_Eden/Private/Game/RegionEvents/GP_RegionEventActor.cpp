#include "Game/RegionEvents/GP_RegionEventActor.h"

#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/DecalComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Game/Corruption/GP_WorldCorruptionComponent.h"
#include "Game/GP_GameState.h"
#include "Game/RegionEvents/GP_RegionEventData.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AGP_RegionEventActor::AGP_RegionEventActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetNetUpdateFrequency(5.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ActivationTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("ActivationTrigger"));
	ActivationTrigger->SetupAttachment(SceneRoot);
	ActivationTrigger->SetSphereRadius(ActivationRadius);
	ActivationTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ActivationTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
	ActivationTrigger->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	ActivationTrigger->SetGenerateOverlapEvents(true);
	ActivationTrigger->ShapeColor = FColor(190, 80, 230);

	// The engine decal is intentionally only a safe fallback; content Blueprints can assign a polished material later.
	EventMarkerDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("EventMarkerDecal"));
	EventMarkerDecal->SetupAttachment(SceneRoot);
	EventMarkerDecal->SetRelativeLocation(FVector(0.0f, 0.0f, 80.0f));
	EventMarkerDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	EventMarkerDecal->DecalSize = MarkerDecalSize;
	EventMarkerDecal->SetFadeScreenSize(0.001f);

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultDecalFinder(
		TEXT("/Engine/EngineMaterials/DefaultDeferredDecalMaterial.DefaultDeferredDecalMaterial"));
	if (DefaultDecalFinder.Succeeded())
	{
		MarkerDecalMaterial = DefaultDecalFinder.Object;
		EventMarkerDecal->SetDecalMaterial(MarkerDecalMaterial);
	}

	// A light and readable label make discovery possible even when the fallback decal blends into terrain.
	EventMarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EventMarkerLight"));
	EventMarkerLight->SetupAttachment(SceneRoot);
	EventMarkerLight->SetRelativeLocation(FVector(0.0f, 0.0f, 160.0f));
	EventMarkerLight->SetIntensity(MarkerLightIntensity);
	EventMarkerLight->SetAttenuationRadius(1000.0f);
	EventMarkerLight->SetCastShadows(false);

	EventMarkerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("EventMarkerText"));
	EventMarkerText->SetupAttachment(SceneRoot);
	EventMarkerText->SetRelativeLocation(FVector(0.0f, 0.0f, 240.0f));
	EventMarkerText->SetHorizontalAlignment(EHTA_Center);
	EventMarkerText->SetVerticalAlignment(EVRTA_TextCenter);
	EventMarkerText->SetWorldSize(48.0f);
	EventMarkerText->SetText(NSLOCTEXT("RegionEvent", "FallbackMarker", "REGION EVENT"));
}

void AGP_RegionEventActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshWorldMarkerPresentation();
}

void AGP_RegionEventActor::BeginPlay()
{
	Super::BeginPlay();

	ActivationTrigger->SetSphereRadius(GetConfiguredApproachRadius());
	RefreshWorldMarkerPresentation();
	if (HasAuthority())
	{
		ActivationTrigger->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::HandleActivationOverlap);
	}
}

void AGP_RegionEventActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
		World->GetTimerManager().ClearTimer(DormantWaitTimerHandle);
	}

	// Remove dynamic bindings explicitly because event enemies can outlive a successfully completed event actor.
	for (AGP_EnemyCharacter* SpawnedEnemy : SpawnedEventEnemies)
	{
		if (IsValid(SpawnedEnemy))
		{
			SpawnedEnemy->OnEnemyDied.RemoveDynamic(this, &ThisClass::HandleSpawnedEventEnemyDied);
		}
	}
	SpawnedEventEnemies.Reset();

	Super::EndPlay(EndPlayReason);
}

void AGP_RegionEventActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AGP_RegionEventActor, RegionId);
	DOREPLIFETIME(AGP_RegionEventActor, EventData);
	DOREPLIFETIME(AGP_RegionEventActor, RuntimeState);
}

void AGP_RegionEventActor::InitializeRegionEvent(int32 InRegionId, UGP_RegionEventData* InEventData)
{
	if (!HasAuthority())
	{
		return;
	}

	// Initialization is idempotent for pooled/test actors and resets every server-side lifecycle timer.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
	}
	StopWaitingForPlayerApproach();
	bApproachActivationRequested = false;

	RegionId = InRegionId;
	EventData = InEventData;
	RuntimeState = EGPRegionEventRuntimeState::Dormant;
	ActivationTrigger->SetSphereRadius(GetConfiguredApproachRadius());
	RefreshWorldMarkerPresentation();

	BP_OnRegionEventInitialized();
	OnRegionEventStateChanged.Broadcast(this);
	ForceNetUpdate();
}

void AGP_RegionEventActor::ConfigureApproachActivation(
	bool bWaitForPlayerApproach,
	float InActivationRadius,
	float InDormantWaitTimeoutSeconds)
{
	if (!HasAuthority())
	{
		return;
	}

	// A runtime override is useful for PCG/director-spawned events while DataAssets retain reusable defaults.
	bHasApproachActivationOverride = true;
	bApproachActivationOverride = bWaitForPlayerApproach;
	ApproachActivationRadiusOverride = FMath::Max(0.0f, InActivationRadius);
	DormantWaitTimeoutOverride = FMath::Max(-1.0f, InDormantWaitTimeoutSeconds);
	ActivationTrigger->SetSphereRadius(GetConfiguredApproachRadius());

	if (RuntimeState == EGPRegionEventRuntimeState::Dormant)
	{
		if (ShouldWaitForPlayerApproach())
		{
			BeginWaitingForPlayerApproach();
		}
		else
		{
			StopWaitingForPlayerApproach();
		}
	}
}

void AGP_RegionEventActor::ConfigureExplorationActivation(float InActivationRadius, float InDormantTimeoutSeconds)
{
	// The director selects exploration events as a single mode, so it should not need to pass a redundant true flag.
	ConfigureApproachActivation(/*bWaitForPlayerApproach=*/true, InActivationRadius, InDormantTimeoutSeconds);
}

void AGP_RegionEventActor::ActivateRegionEvent()
{
	if (!HasAuthority() || RuntimeState != EGPRegionEventRuntimeState::Dormant || !IsValid(EventData))
	{
		return;
	}

	// The director may request activation immediately; discovery-enabled events remain dormant until approached.
	if (ShouldWaitForPlayerApproach() && !bApproachActivationRequested)
	{
		BeginWaitingForPlayerApproach();
		return;
	}

	StopWaitingForPlayerApproach();
	if (EventData->bApplyActiveRegionState)
	{
		ApplyRegionState(EventData->ActiveRegionState);
	}

	SetRuntimeState(EGPRegionEventRuntimeState::Active);
	SpawnConfiguredEnemies();

	// Duration < 0 means Blueprint or director will complete the event explicitly.
	if (EventData->DurationSeconds >= 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				AutoExpireTimerHandle,
				this,
				&ThisClass::ExpireRegionEvent,
				FMath::Max(KINDA_SMALL_NUMBER, EventData->DurationSeconds),
				false);
		}
	}
}

void AGP_RegionEventActor::CompleteRegionEvent()
{
	if (!HasAuthority()
		|| RuntimeState == EGPRegionEventRuntimeState::Completed
		|| RuntimeState == EGPRegionEventRuntimeState::Expired)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
	}
	StopWaitingForPlayerApproach();

	if (IsValid(EventData) && EventData->bApplyCompletedRegionState)
	{
		ApplyRegionState(EventData->CompletedRegionState);
	}
	ApplyCorruptionOutcome(/*bCompletedSuccessfully=*/true);

	SetRuntimeState(EGPRegionEventRuntimeState::Completed);
}

void AGP_RegionEventActor::ExpireRegionEvent()
{
	if (!HasAuthority()
		|| RuntimeState == EGPRegionEventRuntimeState::Completed
		|| RuntimeState == EGPRegionEventRuntimeState::Expired)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoExpireTimerHandle);
	}
	StopWaitingForPlayerApproach();
	ApplyCorruptionOutcome(/*bCompletedSuccessfully=*/false);
	SetRuntimeState(EGPRegionEventRuntimeState::Expired);
}

int32 AGP_RegionEventActor::GetAliveSpawnedEnemyCount() const
{
	int32 AliveEnemyCount = 0;
	for (const AGP_EnemyCharacter* SpawnedEnemy : SpawnedEventEnemies)
	{
		if (IsValid(SpawnedEnemy) && !SpawnedEnemy->IsDead())
		{
			++AliveEnemyCount;
		}
	}
	return AliveEnemyCount;
}

bool AGP_RegionEventActor::IsBlockingZoneCompletion() const
{
	return IsValid(EventData) && EventData->bBlocksZoneCompletion
		&& RuntimeState != EGPRegionEventRuntimeState::Completed
		&& RuntimeState != EGPRegionEventRuntimeState::Expired;
}

void AGP_RegionEventActor::SetRuntimeState(EGPRegionEventRuntimeState NewState)
{
	if (RuntimeState == NewState)
	{
		return;
	}

	RuntimeState = NewState;
	RefreshWorldMarkerPresentation();
	DispatchStatePresentation(RuntimeState);
	OnRegionEventStateChanged.Broadcast(this);
	ForceNetUpdate();
}

void AGP_RegionEventActor::ApplyRegionState(uint8 NewState) const
{
	if (AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr)
	{
		// Region visuals already replicate through AGP_GameState, so event actors only write on the server.
		GPGameState->SetRegionState(RegionId, NewState);
	}
}

void AGP_RegionEventActor::ApplyCorruptionOutcome(bool bCompletedSuccessfully) const
{
	const AGP_GameState* GPGameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr;
	UGP_WorldCorruptionComponent* Corruption = IsValid(GPGameState) ? GPGameState->GetWorldCorruptionComponent() : nullptr;
	if (!IsValid(EventData) || !IsValid(Corruption) || RegionId == INDEX_NONE)
	{
		return;
	}

	// Keep outcome tuning in the DataAsset while the replicated GameState component remains the single source of truth.
	if (bCompletedSuccessfully && EventData->CompletionCorruptionReduction > 0.0f)
	{
		Corruption->ReduceRegionCorruption(RegionId, EventData->CompletionCorruptionReduction);
	}
	else if (!bCompletedSuccessfully && EventData->ExpirationCorruptionIncrease > 0.0f)
	{
		Corruption->AddRegionCorruption(RegionId, EventData->ExpirationCorruptionIncrease);
	}
}

int32 AGP_RegionEventActor::SpawnConfiguredEnemies()
{
	if (!HasAuthority() || !IsValid(EventData))
	{
		return 0;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 SpawnedEnemyCount = 0;
	for (const FGP_EnemySpawnEntry& SpawnEntry : EventData->EnemySpawns)
	{
		if (!*SpawnEntry.EnemyClass)
		{
			continue;
		}

		for (int32 SpawnIndex = 0; SpawnIndex < SpawnEntry.Count; ++SpawnIndex)
		{
			bool bProjected = false;
			const FVector SpawnLocation = ResolveSpawnLocation(EventData->EnemySpawnScatterRadius, bProjected);
			if (!bProjected)
			{
				UE_LOG(LogTemp, Warning, TEXT("[RegionEvent] Skipped enemy spawn for %s: navmesh projection failed."),
					*GetNameSafe(this));
				continue;
			}

			const FTransform SpawnTransform(FRotator::ZeroRotator, SpawnLocation);
			AGP_EnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<AGP_EnemyCharacter>(
				SpawnEntry.EnemyClass,
				SpawnTransform,
				this,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
			if (!IsValid(SpawnedEnemy))
			{
				continue;
			}

			// Assign the region before BeginPlay so corruption scaling never samples the wrong region for one frame.
			SpawnedEnemy->SetCorruptionRegionId(RegionId);
			SpawnedEventEnemies.AddUnique(SpawnedEnemy);
			SpawnedEnemy->OnEnemyDied.AddDynamic(this, &ThisClass::HandleSpawnedEventEnemyDied);
			UGameplayStatics::FinishSpawningActor(SpawnedEnemy, SpawnTransform);
			SpawnedEnemy->SpawnDefaultController();

			++SpawnedEnemyCount;
			OnRegionEventEnemySpawned.Broadcast(this, SpawnedEnemy);
		}
	}

	return SpawnedEnemyCount;
}

FVector AGP_RegionEventActor::ResolveSpawnLocation(float ScatterRadius, bool& bOutProjected) const
{
	const float SafeScatterRadius = FMath::Max(0.0f, ScatterRadius);
	const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
	const float Distance = SafeScatterRadius * FMath::Sqrt(FMath::FRand());
	const FVector DesiredLocation = GetActorLocation()
		+ FVector(FMath::Cos(Angle) * Distance, FMath::Sin(Angle) * Distance, SpawnZOffset);

	bOutProjected = false;
	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(DesiredLocation, ProjectedLocation, NavProjectionExtent))
		{
			bOutProjected = true;
			return ProjectedLocation.Location;
		}
	}

	return DesiredLocation;
}

void AGP_RegionEventActor::OnTrackedEnemyCountChanged()
{
	// Base events only expose the count; wave-based subclasses can complete themselves from this hook.
}

void AGP_RegionEventActor::DispatchStatePresentation(EGPRegionEventRuntimeState PresentedState)
{
	switch (PresentedState)
	{
	case EGPRegionEventRuntimeState::Active:
		BP_OnRegionEventActivated();
		break;
	case EGPRegionEventRuntimeState::Completed:
		BP_OnRegionEventCompleted();
		break;
	case EGPRegionEventRuntimeState::Expired:
		BP_OnRegionEventExpired();
		break;
	default:
		break;
	}
}

void AGP_RegionEventActor::RefreshWorldMarkerPresentation()
{
	if (!IsValid(EventMarkerDecal) || !IsValid(EventMarkerLight) || !IsValid(EventMarkerText))
	{
		return;
	}

	const bool bMarkerVisible = bShowWorldMarker;
	const FLinearColor MarkerColor = GetMarkerColorForState(RuntimeState);

	EventMarkerDecal->SetVisibility(bMarkerVisible, true);
	EventMarkerDecal->DecalSize = MarkerDecalSize.ComponentMax(FVector::ZeroVector);
	EventMarkerDecal->SetDecalMaterial(MarkerDecalMaterial);

	EventMarkerLight->SetVisibility(bMarkerVisible, true);
	EventMarkerLight->SetLightColor(MarkerColor);
	EventMarkerLight->SetIntensity(MarkerLightIntensity);

	EventMarkerText->SetVisibility(bMarkerVisible, true);
	EventMarkerText->SetText(GetMarkerTextForState(RuntimeState));
	EventMarkerText->SetTextRenderColor(MarkerColor.ToFColor(/*bSRGB=*/true));
}

void AGP_RegionEventActor::BeginWaitingForPlayerApproach()
{
	if (!HasAuthority() || RuntimeState != EGPRegionEventRuntimeState::Dormant || !ShouldWaitForPlayerApproach())
	{
		return;
	}

	bWaitingForPlayerApproach = true;
	ActivationTrigger->SetSphereRadius(GetConfiguredApproachRadius());
	TryActivateFromCurrentOverlaps();
	if (!bWaitingForPlayerApproach)
	{
		return;
	}

	const float WaitTimeout = GetConfiguredDormantWaitTimeout();
	if (WaitTimeout >= 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				DormantWaitTimerHandle,
				this,
				&ThisClass::HandleDormantWaitTimeout,
				FMath::Max(KINDA_SMALL_NUMBER, WaitTimeout),
				false);
		}
	}
}

void AGP_RegionEventActor::StopWaitingForPlayerApproach()
{
	bWaitingForPlayerApproach = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(DormantWaitTimerHandle);
	}
}

void AGP_RegionEventActor::TryActivateFromCurrentOverlaps()
{
	if (!bWaitingForPlayerApproach || !IsValid(ActivationTrigger))
	{
		return;
	}

	TArray<AActor*> OverlappingPlayers;
	ActivationTrigger->GetOverlappingActors(OverlappingPlayers, AGP_PlayerCharacter::StaticClass());
	if (!OverlappingPlayers.IsEmpty())
	{
		bApproachActivationRequested = true;
		ActivateRegionEvent();
	}
}

void AGP_RegionEventActor::HandleDormantWaitTimeout()
{
	if (HasAuthority() && bWaitingForPlayerApproach && RuntimeState == EGPRegionEventRuntimeState::Dormant)
	{
		// Timing out an ignored discovery is a normal event failure and applies the configured failure consequence.
		ExpireRegionEvent();
	}
}

bool AGP_RegionEventActor::ShouldWaitForPlayerApproach() const
{
	if (bHasApproachActivationOverride)
	{
		return bApproachActivationOverride;
	}

	return bActivateOnPlayerOverlap || (IsValid(EventData) && EventData->bActivateWhenPlayerApproaches);
}

float AGP_RegionEventActor::GetConfiguredApproachRadius() const
{
	if (bHasApproachActivationOverride)
	{
		return FMath::Max(0.0f, ApproachActivationRadiusOverride);
	}

	if (IsValid(EventData) && EventData->bActivateWhenPlayerApproaches)
	{
		return FMath::Max(0.0f, EventData->ApproachActivationRadius);
	}

	return FMath::Max(0.0f, ActivationRadius);
}

float AGP_RegionEventActor::GetConfiguredDormantWaitTimeout() const
{
	if (bHasApproachActivationOverride)
	{
		return FMath::Max(-1.0f, DormantWaitTimeoutOverride);
	}

	return IsValid(EventData) ? FMath::Max(-1.0f, EventData->DormantWaitTimeoutSeconds) : -1.0f;
}

FLinearColor AGP_RegionEventActor::GetMarkerColorForState(EGPRegionEventRuntimeState State) const
{
	switch (State)
	{
	case EGPRegionEventRuntimeState::Active:
		return ActiveMarkerColor;
	case EGPRegionEventRuntimeState::Completed:
		return CompletedMarkerColor;
	case EGPRegionEventRuntimeState::Expired:
		return ExpiredMarkerColor;
	default:
		return DormantMarkerColor;
	}
}

FText AGP_RegionEventActor::GetMarkerTextForState(EGPRegionEventRuntimeState State) const
{
	const FText EventName = IsValid(EventData) && !EventData->DisplayName.IsEmpty()
		? EventData->DisplayName
		: NSLOCTEXT("RegionEvent", "FallbackEventName", "REGION EVENT");

	switch (State)
	{
	case EGPRegionEventRuntimeState::Active:
		return FText::Format(NSLOCTEXT("RegionEvent", "ActiveMarker", "{0}\nACTIVE"), EventName);
	case EGPRegionEventRuntimeState::Completed:
		return FText::Format(NSLOCTEXT("RegionEvent", "CompletedMarker", "{0}\nCLEARED"), EventName);
	case EGPRegionEventRuntimeState::Expired:
		return FText::Format(NSLOCTEXT("RegionEvent", "ExpiredMarker", "{0}\nFAILED"), EventName);
	default:
		return FText::Format(NSLOCTEXT("RegionEvent", "DormantMarker", "{0}\nAPPROACH TO START"), EventName);
	}
}

void AGP_RegionEventActor::OnRep_EventData()
{
	ActivationTrigger->SetSphereRadius(GetConfiguredApproachRadius());
	RefreshWorldMarkerPresentation();
}

void AGP_RegionEventActor::OnRep_RuntimeState(EGPRegionEventRuntimeState PreviousState)
{
	if (PreviousState != RuntimeState)
	{
		RefreshWorldMarkerPresentation();
		DispatchStatePresentation(RuntimeState);
		OnRegionEventStateChanged.Broadcast(this);
	}
}

void AGP_RegionEventActor::HandleSpawnedEventEnemyDied(AGP_EnemyCharacter* DeadEnemy)
{
	if (!HasAuthority() || !IsValid(DeadEnemy))
	{
		return;
	}

	DeadEnemy->OnEnemyDied.RemoveDynamic(this, &ThisClass::HandleSpawnedEventEnemyDied);
	SpawnedEventEnemies.Remove(DeadEnemy);
	OnTrackedEnemyCountChanged();
}

void AGP_RegionEventActor::HandleActivationOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!ShouldWaitForPlayerApproach()
		|| RuntimeState != EGPRegionEventRuntimeState::Dormant
		|| !Cast<AGP_PlayerCharacter>(OtherActor))
	{
		return;
	}

	bApproachActivationRequested = true;
	ActivateRegionEvent();
}
