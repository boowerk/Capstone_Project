#include "Game/Demo/GP_DemoRunDirector.h"

#include "AI/Controllers/EnemyAIController.h"
#include "BrainComponent.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Game/GP_GameMode.h"
#include "Game/GP_GameState.h"
#include "Game/RegionEvents/GP_RegionEventActor.h"
#include "Game/RegionEvents/GP_RegionEventDirector.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName RedRiftEventId(TEXT("world_red_rift"));
	const FName StructureDefenseEventId(TEXT("world_structure_defense"));
	const FName ShrineEventId(TEXT("world_shrine_ruins"));
}

AGP_DemoRunDirector::AGP_DemoRunDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ObjectiveLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ObjectiveLight"));
	ObjectiveLight->SetupAttachment(SceneRoot);
	ObjectiveLight->SetRelativeLocation(FVector(0.0f, 0.0f, 220.0f));
	ObjectiveLight->SetLightColor(FLinearColor(1.0f, 0.58f, 0.08f));
	ObjectiveLight->SetIntensity(5000.0f);
	ObjectiveLight->SetAttenuationRadius(1400.0f);
	ObjectiveLight->SetCastShadows(false);

	ObjectiveText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("ObjectiveText"));
	ObjectiveText->SetupAttachment(SceneRoot);
	ObjectiveText->SetRelativeLocation(FVector(0.0f, 0.0f, 340.0f));
	ObjectiveText->SetHorizontalAlignment(EHTA_Center);
	ObjectiveText->SetVerticalAlignment(EVRTA_TextCenter);
	ObjectiveText->SetWorldSize(64.0f);
	ObjectiveText->SetTextRenderColor(FColor(255, 185, 45));
	ObjectiveText->SetText(NSLOCTEXT("DemoRun", "WaitingMarker", "DEMO ROUTE\nASSEMBLING PARTY"));

	static ConstructorHelpers::FClassFinder<AGP_EnemyCharacter> DarkArmorKnightFinder(
		TEXT("/Game/Characters/EnemyCharacter/Boss/BP_Boss_DarkArmorKnight/BP_DarkArmorKnight"));
	if (DarkArmorKnightFinder.Succeeded())
	{
		DarkArmorKnightClass = DarkArmorKnightFinder.Class;
	}
}

void AGP_DemoRunDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AGP_DemoRunDirector, CurrentStage);
	DOREPLIFETIME(AGP_DemoRunDirector, ObjectiveLocation);
	DOREPLIFETIME(AGP_DemoRunDirector, ObjectiveRegionId);
	DOREPLIFETIME(AGP_DemoRunDirector, ObjectiveEventId);
	DOREPLIFETIME(AGP_DemoRunDirector, RunSeed);
}

bool AGP_DemoRunDirector::InitializeDemoRun(
	AGP_GameMode* InGameMode,
	AGP_RegionEventDirector* InRegionEventDirector,
	const FGPDemoRunRoute& InRoute,
	int32 InRunSeed)
{
	if (!HasAuthority()
		|| bInitialized
		|| !IsValid(InGameMode)
		|| !IsValid(InRegionEventDirector)
		|| !InRoute.IsValid()
		|| !GPDemoRunFlowPolicy::IsStrictlyInward(InRoute))
	{
		return false;
	}

	OwningGameMode = InGameMode;
	RegionEventDirector = InRegionEventDirector;
	Route = InRoute;
	RunSeed = InRunSeed;
	bInitialized = true;
	// The event coordinator resolves explicit demo IDs only; no ambient scheduler is active.
	RegionEventDirector->OnRegionEventEnded.AddDynamic(this, &ThisClass::HandleGuidedEventEnded);

	const FGPDemoRoutePoint* OpeningPoint = Route.FindPoint(EGPDemoRouteRole::OpeningObjective);
	SetFlowStage(EGPDemoRunStage::WaitingForParty, OpeningPoint, RedRiftEventId);
	PartyAssemblyStartedAtSeconds = GetWorldTimeSeconds();
	if (AGP_GameState* GameState = GetWorld() ? GetWorld()->GetGameState<AGP_GameState>() : nullptr)
	{
		GameState->SetMatchPhase(EGPMatchPhase::InCity);
	}

	GetWorldTimerManager().SetTimer(
		FlowEvaluationTimerHandle,
		this,
		&ThisClass::EvaluateFlow,
		FMath::Max(0.1f, FlowEvaluationIntervalSeconds),
		true);
	EvaluateFlow();
	return true;
}

void AGP_DemoRunDirector::StopDemoRun()
{
	if (!HasAuthority() || !bInitialized || bFinishingRun)
	{
		return;
	}

	bFinishingRun = true;
	GetWorldTimerManager().ClearTimer(FlowEvaluationTimerHandle);
	if (IsValid(RegionEventDirector))
	{
		RegionEventDirector->OnRegionEventEnded.RemoveDynamic(this, &ThisClass::HandleGuidedEventEnded);
	}
	if (IsValid(CurrentEvent)
		&& CurrentEvent->GetRuntimeState() != EGPRegionEventRuntimeState::Completed
		&& CurrentEvent->GetRuntimeState() != EGPRegionEventRuntimeState::Expired)
	{
		// Expire the authored beat before result travel so it cannot keep spawning gameplay actors.
		CurrentEvent->ExpireRegionEvent();
	}
	CurrentEvent = nullptr;
	if (IsValid(CurrentBoss))
	{
		CurrentBoss->OnEnemyDeathStarted.RemoveDynamic(this, &ThisClass::HandleBossDeathStarted);
		CurrentBoss->Destroy();
	}
	CurrentBoss = nullptr;

	// Completed is the replicated terminal presentation; GameState still distinguishes victory from defeat.
	CurrentStage = EGPDemoRunStage::Completed;
	ObjectiveEventId = NAME_None;
	RefreshMarkerPresentation();
	ForceNetUpdate();
}

void AGP_DemoRunDirector::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(FlowEvaluationTimerHandle);
	if (IsValid(RegionEventDirector))
	{
		RegionEventDirector->OnRegionEventEnded.RemoveDynamic(this, &ThisClass::HandleGuidedEventEnded);
	}
	if (IsValid(CurrentBoss))
	{
		CurrentBoss->OnEnemyDeathStarted.RemoveDynamic(this, &ThisClass::HandleBossDeathStarted);
	}
	Super::EndPlay(EndPlayReason);
}

FName AGP_DemoRunDirector::GetEventIdForStage(EGPDemoRunStage Stage)
{
	switch (Stage)
	{
	case EGPDemoRunStage::OpeningRedRift:
		return RedRiftEventId;
	case EGPDemoRunStage::PressureDefense:
		return StructureDefenseEventId;
	case EGPDemoRunStage::InnerShrine:
		return ShrineEventId;
	default:
		return NAME_None;
	}
}

EGPDemoRouteRole AGP_DemoRunDirector::GetRouteRoleForStage(EGPDemoRunStage Stage)
{
	switch (Stage)
	{
	case EGPDemoRunStage::OpeningRedRift:
		return EGPDemoRouteRole::OpeningObjective;
	case EGPDemoRunStage::PressureDefense:
		return EGPDemoRouteRole::PressureObjective;
	case EGPDemoRunStage::InnerShrine:
	case EGPDemoRunStage::RewardGrace:
		return EGPDemoRouteRole::InnerReward;
	case EGPDemoRunStage::CenterRally:
	case EGPDemoRunStage::BossFight:
	case EGPDemoRunStage::Completed:
		return EGPDemoRouteRole::CenterBoss;
	default:
		return EGPDemoRouteRole::OpeningObjective;
	}
}

EGPDemoRunStage AGP_DemoRunDirector::GetNextStageAfterEvent(EGPDemoRunStage Stage)
{
	switch (Stage)
	{
	case EGPDemoRunStage::OpeningRedRift:
		return EGPDemoRunStage::PressureDefense;
	case EGPDemoRunStage::PressureDefense:
		return EGPDemoRunStage::InnerShrine;
	case EGPDemoRunStage::InnerShrine:
		return EGPDemoRunStage::RewardGrace;
	default:
		return Stage;
	}
}

void AGP_DemoRunDirector::EvaluateFlow()
{
	if (!HasAuthority() || !bInitialized || bFinishingRun)
	{
		return;
	}

	const double Now = GetWorldTimeSeconds();
	switch (CurrentStage)
	{
	case EGPDemoRunStage::WaitingForParty:
	{
		const int32 PlayerCount = CountPossessedPlayers();
		if (PlayerCount >= FMath::Max(1, PreferredPartyPlayerCount)
			|| (PlayerCount > 0 && Now - PartyAssemblyStartedAtSeconds >= PartyAssemblyTimeoutSeconds))
		{
			BeginEventStage(EGPDemoRunStage::OpeningRedRift);
		}
		break;
	}
	case EGPDemoRunStage::OpeningRedRift:
	case EGPDemoRunStage::PressureDefense:
	case EGPDemoRunStage::InnerShrine:
		if (IsValid(CurrentEvent))
		{
			if (Now - StageStartedAtSeconds >= GuidedEventWatchdogSeconds)
			{
				// Expiration closes a stalled scripted beat, then the exact-event callback keeps the demo moving.
				CurrentEvent->ExpireRegionEvent();
			}
		}
		else if (Now - StageStartedAtSeconds >= GuidedEventWatchdogSeconds)
		{
			// Temporary scripted-event conflicts may clear themselves; only the stage watchdog may skip a beat.
			UE_LOG(LogTemp, Error, TEXT("[DemoFlow] Skipping stage %d after its spawn watchdog elapsed."), static_cast<int32>(CurrentStage));
			AdvanceFromCurrentEventStage();
		}
		else if (Now >= NextSpawnAttemptAtSeconds)
		{
			TrySpawnCurrentEventStage();
		}
		break;
	case EGPDemoRunStage::RewardGrace:
		if (Now - StageStartedAtSeconds >= RewardPresentationGraceSeconds)
		{
			BeginCenterRally();
		}
		break;
	case EGPDemoRunStage::CenterRally:
	{
		const int32 PossessedPlayers = CountPossessedPlayers();
		const int32 RequiredPlayers = FMath::Clamp(2, 1, FMath::Max(1, PossessedPlayers));
		const int32 NearbyPlayers = CountPlayersNearObjective();
		if (NearbyPlayers >= RequiredPlayers)
		{
			if (RallySatisfiedAtSeconds < 0.0)
			{
				RallySatisfiedAtSeconds = Now;
			}
			if (Now - RallySatisfiedAtSeconds >= CenterRallyHoldSeconds)
			{
				TryStartBossFight();
			}
		}
		else
		{
			RallySatisfiedAtSeconds = -1.0;
			if (NearbyPlayers > 0 && Now - StageStartedAtSeconds >= CenterRallyWatchdogSeconds)
			{
				// Keep the showcase moving when one connected teammate cannot reach the final rally.
				UE_LOG(LogTemp, Warning, TEXT("[DemoFlow] Center rally watchdog accepted one nearby player."));
				TryStartBossFight();
			}
		}
		break;
	}
	case EGPDemoRunStage::BossFight:
		if (!IsValid(CurrentBoss) && Now >= NextSpawnAttemptAtSeconds)
		{
			TryStartBossFight();
		}
		break;
	default:
		break;
	}
}

void AGP_DemoRunDirector::BeginEventStage(EGPDemoRunStage NewStage)
{
	const FGPDemoRoutePoint* RoutePoint = Route.FindPoint(GetRouteRoleForStage(NewStage));
	SetFlowStage(NewStage, RoutePoint, GetEventIdForStage(NewStage));
	TrySpawnCurrentEventStage();
}

void AGP_DemoRunDirector::TrySpawnCurrentEventStage()
{
	if (!IsValid(RegionEventDirector)
		|| GetEventIdForStage(CurrentStage).IsNone()
		|| ObjectiveRegionId == INDEX_NONE)
	{
		AdvanceFromCurrentEventStage();
		return;
	}

	++StageSpawnAttemptCount;
	AGP_RegionEventActor* SpawnedEvent = RegionEventDirector->TryStartGuidedRegionEventAtLocation(
		GetEventIdForStage(CurrentStage),
		ObjectiveRegionId,
		ObjectiveLocation,
		/*DormantWaitTimeoutSeconds=*/-1.0f,
		/*RequiredApproachPlayers=*/2);
	CurrentEvent = SpawnedEvent;
	if (IsValid(CurrentEvent))
	{
		// A custom event may terminate synchronously before the assignment above can satisfy the exact-pointer callback.
		if (CurrentEvent->GetRuntimeState() == EGPRegionEventRuntimeState::Completed
			|| CurrentEvent->GetRuntimeState() == EGPRegionEventRuntimeState::Expired)
		{
			AdvanceFromCurrentEventStage();
		}
		return;
	}

	const int32 WarningInterval = FMath::Max(1, SpawnFailureWarningInterval);
	if (StageSpawnAttemptCount % WarningInterval == 0)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[DemoFlow] Stage %d has failed to spawn %d times; retrying until its watchdog elapses."),
			static_cast<int32>(CurrentStage),
			StageSpawnAttemptCount);
	}
	NextSpawnAttemptAtSeconds = GetWorldTimeSeconds() + 5.0;
}

void AGP_DemoRunDirector::AdvanceFromCurrentEventStage()
{
	CurrentEvent = nullptr;
	const EGPDemoRunStage NextStage = GetNextStageAfterEvent(CurrentStage);
	if (NextStage == EGPDemoRunStage::RewardGrace)
	{
		BeginRewardGrace();
	}
	else if (NextStage != CurrentStage)
	{
		BeginEventStage(NextStage);
	}
}

void AGP_DemoRunDirector::BeginRewardGrace()
{
	SetFlowStage(
		EGPDemoRunStage::RewardGrace,
		Route.FindPoint(EGPDemoRouteRole::InnerReward),
		NAME_None);
}

void AGP_DemoRunDirector::BeginCenterRally()
{
	RallySatisfiedAtSeconds = -1.0;
	SetFlowStage(
		EGPDemoRunStage::CenterRally,
		Route.FindPoint(EGPDemoRouteRole::CenterBoss),
		NAME_None);
}

void AGP_DemoRunDirector::TryStartBossFight()
{
	UWorld* World = GetWorld();
	if (!HasAuthority() || !IsValid(World) || IsValid(CurrentBoss) || !*DarkArmorKnightClass)
	{
		if (!*DarkArmorKnightClass)
		{
			NextSpawnAttemptAtSeconds = GetWorldTimeSeconds() + 5.0;
		}
		return;
	}
	if (GetWorldTimeSeconds() < NextSpawnAttemptAtSeconds)
	{
		return;
	}

	FVector DesiredBossLocation = ObjectiveLocation;
	if (const FGPDemoRoutePoint* InnerRewardPoint = Route.FindPoint(EGPDemoRouteRole::InnerReward))
	{
		const FVector RevealDirection = (ObjectiveLocation - InnerRewardPoint->WorldLocation).GetSafeNormal2D();
		if (!RevealDirection.IsNearlyZero())
		{
			DesiredBossLocation += RevealDirection * FMath::Max(0.0f, BossRevealOffsetDistance);
		}
	}
	FVector BossLocation = DesiredBossLocation;
	ResolveGroundedObjectiveLocation(DesiredBossLocation, BossLocation);
	FRotator BossRotation = FRotator::ZeroRotator;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		const APawn* PlayerPawn = Iterator->Get() ? Iterator->Get()->GetPawn() : nullptr;
		if (IsValid(PlayerPawn))
		{
			BossRotation = (PlayerPawn->GetActorLocation() - BossLocation).Rotation();
			BossRotation.Pitch = 0.0f;
			BossRotation.Roll = 0.0f;
			break;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	SpawnParameters.Owner = this;
	CurrentBoss = World->SpawnActor<AGP_EnemyCharacter>(
		DarkArmorKnightClass,
		BossLocation,
		BossRotation,
		SpawnParameters);
	if (!IsValid(CurrentBoss))
	{
		NextSpawnAttemptAtSeconds = GetWorldTimeSeconds() + 5.0;
		return;
	}

	CurrentBoss->OnEnemyDeathStarted.AddDynamic(this, &ThisClass::HandleBossDeathStarted);
	if (!CurrentBoss->GetController())
	{
		CurrentBoss->SpawnDefaultController();
	}
	const AEnemyAIController* BossController = Cast<AEnemyAIController>(CurrentBoss->GetController());
	const UBrainComponent* BossBrain = IsValid(BossController) ? BossController->GetBrainComponent() : nullptr;
	if (!IsValid(BossController) || !IsValid(BossBrain) || !BossBrain->IsRunning())
	{
		UE_LOG(LogTemp, Error, TEXT("[DemoFlow] Dark Armor Knight AI failed to initialize; retrying the boss reveal."));
		CurrentBoss->OnEnemyDeathStarted.RemoveDynamic(this, &ThisClass::HandleBossDeathStarted);
		CurrentBoss->Destroy();
		CurrentBoss = nullptr;
		NextSpawnAttemptAtSeconds = GetWorldTimeSeconds() + 5.0;
		return;
	}
	SetFlowStage(
		EGPDemoRunStage::BossFight,
		Route.FindPoint(EGPDemoRouteRole::CenterBoss),
		NAME_None);
	if (AGP_GameState* GameState = World->GetGameState<AGP_GameState>())
	{
		GameState->SetMatchPhase(EGPMatchPhase::BossFight);
	}
	UE_LOG(LogTemp, Display, TEXT("[DemoFlow] Dark Armor Knight spawned in center region %d."), ObjectiveRegionId);
}

void AGP_DemoRunDirector::FinishDemoRun()
{
	if (bFinishingRun)
	{
		return;
	}
	bFinishingRun = true;
	SetFlowStage(
		EGPDemoRunStage::Completed,
		Route.FindPoint(EGPDemoRouteRole::CenterBoss),
		NAME_None);
	GetWorldTimerManager().ClearTimer(FlowEvaluationTimerHandle);
	if (IsValid(OwningGameMode))
	{
		OwningGameMode->CompleteGuidedDemoRun(true);
	}
}

void AGP_DemoRunDirector::SetFlowStage(
	EGPDemoRunStage NewStage,
	const FGPDemoRoutePoint* RoutePoint,
	FName EventId)
{
	CurrentStage = NewStage;
	ObjectiveEventId = EventId;
	ObjectiveRegionId = RoutePoint ? RoutePoint->RegionId : INDEX_NONE;
	ObjectiveLocation = RoutePoint ? RoutePoint->WorldLocation : GetActorLocation();
	FVector GroundedLocation = ObjectiveLocation;
	if (ResolveGroundedObjectiveLocation(ObjectiveLocation, GroundedLocation))
	{
		ObjectiveLocation = GroundedLocation;
	}
	SetActorLocation(ObjectiveLocation);
	StageStartedAtSeconds = GetWorldTimeSeconds();
	NextSpawnAttemptAtSeconds = StageStartedAtSeconds;
	StageSpawnAttemptCount = 0;
	RefreshMarkerPresentation();
	ForceNetUpdate();

	UE_LOG(
		LogTemp,
		Display,
		TEXT("[DemoFlow] Stage=%d Region=%d Objective=%s Seed=%d"),
		static_cast<int32>(CurrentStage),
		ObjectiveRegionId,
		*ObjectiveLocation.ToCompactString(),
		RunSeed);
}

bool AGP_DemoRunDirector::ResolveGroundedObjectiveLocation(
	const FVector& DesiredLocation,
	FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World || DesiredLocation.ContainsNaN())
	{
		return false;
	}

	if (const UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World))
	{
		FNavLocation ProjectedLocation;
		if (NavigationSystem->ProjectPointToNavigation(
			DesiredLocation,
			ProjectedLocation,
			FVector(1200.0f, 1200.0f, 3000.0f)))
		{
			OutLocation = ProjectedLocation.Location;
			return true;
		}
	}

	FHitResult GroundHit;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(GP_DemoObjectiveGround), false, this);
	if (World->LineTraceSingleByObjectType(
		GroundHit,
		DesiredLocation + FVector(0.0f, 0.0f, 6000.0f),
		DesiredLocation - FVector(0.0f, 0.0f, 10000.0f),
		FCollisionObjectQueryParams(ECC_WorldStatic),
		QueryParams))
	{
		OutLocation = GroundHit.ImpactPoint;
		return true;
	}
	return false;
}

int32 AGP_DemoRunDirector::CountPossessedPlayers() const
{
	int32 PlayerCount = 0;
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			const APlayerController* PlayerController = Iterator->Get();
			PlayerCount += IsValid(PlayerController) && IsValid(PlayerController->GetPawn()) ? 1 : 0;
		}
	}
	return PlayerCount;
}

int32 AGP_DemoRunDirector::CountPlayersNearObjective() const
{
	int32 NearbyPlayerCount = 0;
	if (const UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			const APlayerController* PlayerController = Iterator->Get();
			const APawn* PlayerPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
			NearbyPlayerCount += IsValid(PlayerPawn)
				&& FVector::Dist2D(PlayerPawn->GetActorLocation(), ObjectiveLocation) <= CenterRallyRadius
				? 1
				: 0;
		}
	}
	return NearbyPlayerCount;
}

double AGP_DemoRunDirector::GetWorldTimeSeconds() const
{
	return GetWorld() ? static_cast<double>(GetWorld()->GetTimeSeconds()) : 0.0;
}

void AGP_DemoRunDirector::RefreshMarkerPresentation()
{
	if (!ObjectiveLight || !ObjectiveText)
	{
		return;
	}

	const bool bVisible = CurrentStage != EGPDemoRunStage::BossFight
		&& CurrentStage != EGPDemoRunStage::Completed;
	ObjectiveLight->SetVisibility(bVisible, true);
	ObjectiveText->SetVisibility(bVisible, true);
	ObjectiveText->SetText(GetStageMarkerText());
}

FText AGP_DemoRunDirector::GetStageMarkerText() const
{
	switch (CurrentStage)
	{
	case EGPDemoRunStage::OpeningRedRift:
		return NSLOCTEXT("DemoRun", "RedRiftMarker", "1 / 4\nRED RIFT");
	case EGPDemoRunStage::PressureDefense:
		return NSLOCTEXT("DemoRun", "DefenseMarker", "2 / 4\nSTRUCTURE DEFENSE");
	case EGPDemoRunStage::InnerShrine:
		return NSLOCTEXT("DemoRun", "ShrineMarker", "3 / 4\nANCIENT SHRINE");
	case EGPDemoRunStage::RewardGrace:
		return NSLOCTEXT("DemoRun", "RewardMarker", "AUGMENT SELECT\nPREPARE TO MOVE");
	case EGPDemoRunStage::CenterRally:
		return NSLOCTEXT("DemoRun", "CenterMarker", "4 / 4\nRALLY AT THE CENTER");
	case EGPDemoRunStage::Completed:
		return NSLOCTEXT("DemoRun", "CompletedMarker", "DEMO COMPLETE");
	default:
		return NSLOCTEXT("DemoRun", "WaitingMarker", "DEMO ROUTE\nASSEMBLING PARTY");
	}
}

void AGP_DemoRunDirector::OnRep_FlowPresentation()
{
	SetActorLocation(ObjectiveLocation);
	RefreshMarkerPresentation();
}

void AGP_DemoRunDirector::HandleGuidedEventEnded(
	AGP_RegionEventDirector* Director,
	AGP_RegionEventActor* EventActor)
{
	if (!HasAuthority()
		|| bFinishingRun
		|| Director != RegionEventDirector
		|| !IsValid(EventActor)
		|| EventActor != CurrentEvent)
	{
		return;
	}

	AdvanceFromCurrentEventStage();
}

void AGP_DemoRunDirector::HandleBossDeathStarted(
	AGP_EnemyCharacter* Enemy,
	AActor* InstigatorActor)
{
	if (!HasAuthority() || !IsValid(Enemy) || Enemy != CurrentBoss)
	{
		return;
	}
	Enemy->OnEnemyDeathStarted.RemoveDynamic(this, &ThisClass::HandleBossDeathStarted);
	CurrentBoss = nullptr;
	FinishDemoRun();
}
