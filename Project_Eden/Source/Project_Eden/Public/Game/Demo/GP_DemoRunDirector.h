#pragma once

#include "CoreMinimal.h"
#include "Game/Demo/GP_DemoRunFlowPolicy.h"
#include "GameFramework/Actor.h"
#include "GP_DemoRunDirector.generated.h"

class AGP_EnemyCharacter;
class AGP_GameMode;
class AGP_RegionEventActor;
class AGP_RegionEventDirector;
class UPointLightComponent;
class USceneComponent;
class UTextRenderComponent;

/** Stable authored beats; their route locations vary with the replicated run seed. */
UENUM(BlueprintType)
enum class EGPDemoRunStage : uint8
{
	WaitingForParty UMETA(DisplayName = "Waiting For Party"),
	OpeningRedRift UMETA(DisplayName = "Opening Red Rift"),
	PressureDefense UMETA(DisplayName = "Structure Defense"),
	InnerShrine UMETA(DisplayName = "Inner Shrine"),
	RewardGrace UMETA(DisplayName = "Reward Grace"),
	CenterRally UMETA(DisplayName = "Center Rally"),
	BossFight UMETA(DisplayName = "Dark Armor Knight"),
	Completed UMETA(DisplayName = "Completed")
};

/**
 * Replicated authority actor that layers the graduation-demo golden path over the existing landscape systems.
 * It never edits the map or starts the legacy zero-zone run.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_DemoRunDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_DemoRunDirector();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	bool InitializeDemoRun(
		AGP_GameMode* InGameMode,
		AGP_RegionEventDirector* InRegionEventDirector,
		const FGPDemoRunRoute& InRoute,
		int32 InRunSeed);

	// GameMode uses this terminal path on party defeat so no delayed beat can overwrite the result.
	void StopDemoRun();

	UFUNCTION(BlueprintPure, Category = "Demo Run")
	EGPDemoRunStage GetCurrentStage() const { return CurrentStage; }

	UFUNCTION(BlueprintPure, Category = "Demo Run")
	FVector GetObjectiveLocation() const { return ObjectiveLocation; }

	UFUNCTION(BlueprintPure, Category = "Demo Run")
	int32 GetObjectiveRegionId() const { return ObjectiveRegionId; }

	UFUNCTION(BlueprintPure, Category = "Demo Run")
	int32 GetRunSeed() const { return RunSeed; }

	static FName GetEventIdForStage(EGPDemoRunStage Stage);
	static EGPDemoRouteRole GetRouteRoleForStage(EGPDemoRunStage Stage);
	static EGPDemoRunStage GetNextStageAfterEvent(EGPDemoRunStage Stage);

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run|Marker")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run|Marker")
	TObjectPtr<UPointLightComponent> ObjectiveLight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Demo Run|Marker")
	TObjectPtr<UTextRenderComponent> ObjectiveText;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "1"))
	int32 PreferredPartyPlayerCount = 3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "0.0", Units = "s"))
	float PartyAssemblyTimeoutSeconds = 8.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "0.1", Units = "s"))
	float FlowEvaluationIntervalSeconds = 0.5f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "1.0", Units = "s"))
	float GuidedEventWatchdogSeconds = 180.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "0.0", Units = "s"))
	float RewardPresentationGraceSeconds = 12.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "100.0", Units = "cm"))
	float CenterRallyRadius = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "0.0", Units = "s"))
	float CenterRallyHoldSeconds = 1.0f;

	// After this cap, one player at the center may preserve demo forward progress if a teammate is delayed.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "1.0", Units = "s"))
	float CenterRallyWatchdogSeconds = 45.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Flow", meta = (ClampMin = "1"))
	int32 SpawnFailureWarningInterval = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Boss")
	TSubclassOf<AGP_EnemyCharacter> DarkArmorKnightClass;

	// Reveal the boss just beyond the rally point so it never materializes inside the gathered party.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Demo Run|Boss", meta = (ClampMin = "0.0", Units = "cm"))
	float BossRevealOffsetDistance = 700.0f;

private:
	UPROPERTY(ReplicatedUsing = OnRep_FlowPresentation)
	EGPDemoRunStage CurrentStage = EGPDemoRunStage::WaitingForParty;

	UPROPERTY(ReplicatedUsing = OnRep_FlowPresentation)
	FVector ObjectiveLocation = FVector::ZeroVector;

	UPROPERTY(ReplicatedUsing = OnRep_FlowPresentation)
	int32 ObjectiveRegionId = INDEX_NONE;

	UPROPERTY(ReplicatedUsing = OnRep_FlowPresentation)
	FName ObjectiveEventId = NAME_None;

	UPROPERTY(Replicated)
	int32 RunSeed = 0;

	UPROPERTY(Transient)
	FGPDemoRunRoute Route;

	UPROPERTY(Transient)
	TObjectPtr<AGP_GameMode> OwningGameMode;

	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventDirector> RegionEventDirector;

	UPROPERTY(Transient)
	TObjectPtr<AGP_RegionEventActor> CurrentEvent;

	UPROPERTY(Transient)
	TObjectPtr<AGP_EnemyCharacter> CurrentBoss;

	FTimerHandle FlowEvaluationTimerHandle;
	double StageStartedAtSeconds = 0.0;
	double PartyAssemblyStartedAtSeconds = 0.0;
	double RallySatisfiedAtSeconds = -1.0;
	double NextSpawnAttemptAtSeconds = 0.0;
	int32 StageSpawnAttemptCount = 0;
	bool bInitialized = false;
	bool bFinishingRun = false;

	void EvaluateFlow();
	void BeginEventStage(EGPDemoRunStage NewStage);
	void TrySpawnCurrentEventStage();
	void AdvanceFromCurrentEventStage();
	void BeginRewardGrace();
	void BeginCenterRally();
	void TryStartBossFight();
	void FinishDemoRun();
	void SetFlowStage(EGPDemoRunStage NewStage, const FGPDemoRoutePoint* RoutePoint, FName EventId);
	bool ResolveGroundedObjectiveLocation(const FVector& DesiredLocation, FVector& OutLocation) const;
	int32 CountPossessedPlayers() const;
	int32 CountPlayersNearObjective() const;
	double GetWorldTimeSeconds() const;
	void RefreshMarkerPresentation();
	FText GetStageMarkerText() const;

	UFUNCTION()
	void OnRep_FlowPresentation();

	UFUNCTION()
	void HandleGuidedEventEnded(AGP_RegionEventDirector* Director, AGP_RegionEventActor* EventActor);

	UFUNCTION()
	void HandleBossDeathStarted(AGP_EnemyCharacter* Enemy, AActor* InstigatorActor);

#if WITH_DEV_AUTOMATION_TESTS
	friend class FGPDemoRunGoldenPathContractTest;
#endif
};
