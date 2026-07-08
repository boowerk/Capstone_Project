#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "GP_EncounterDebugDirector.generated.h"

class AGP_EnemyCharacter;
class AGP_PlayerCharacter;
class UGP_EncounterDebugRuntimeWidget;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EGPEncounterDebugSpawnKind : uint8
{
	Mob UMETA(DisplayName = "Mob"),
	Boss UMETA(DisplayName = "Boss")
};

UENUM(BlueprintType)
enum class EGPEncounterDebugReportMode : uint8
{
	Summary UMETA(DisplayName = "Summary"),
	AI UMETA(DisplayName = "AI"),
	Skill UMETA(DisplayName = "Skill"),
	GAS UMETA(DisplayName = "GAS"),
	Matador UMETA(DisplayName = "Matador"),
	All UMETA(DisplayName = "All")
};

USTRUCT(BlueprintType)
struct FGPEncounterDebugStageSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter Debug")
	FName StageName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter Debug")
	TObjectPtr<AActor> StagePoint = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter Debug")
	FTransform FallbackTransform = FTransform::Identity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter Debug")
	TSubclassOf<AGP_EnemyCharacter> MobClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Encounter Debug")
	TSubclassOf<AGP_EnemyCharacter> BossClass;
};

USTRUCT(BlueprintType)
struct FGPEncounterDebugEnemySnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	TObjectPtr<AGP_EnemyCharacter> Enemy = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	FString ClassName;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	int32 StageIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	bool bBoss = false;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	bool bDead = false;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	bool bDebugSpawned = false;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	float Health = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	float HealthPercent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	float DistanceToPlayer = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Encounter Debug")
	FString ControllerName;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPEncounterDebugLogMessage, const FString&, Message);

/**
 * PIE-only encounter test bridge for Editor Utility Widgets.
 *
 * Place a BP child in the test level, assign three stage slots, then let an EUW call these
 * BlueprintCallable functions. Runtime combat logic stays here so the EUW never has to guess
 * which PIE world or actor instance it should touch.
 */
UCLASS(Blueprintable)
class PROJECT_EDEN_API AGP_EncounterDebugDirector : public AActor
{
	GENERATED_BODY()

public:
	AGP_EncounterDebugDirector();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Session")
	static AGP_EncounterDebugDirector* FindActiveEncounterDebugDirector();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Session")
	bool ValidateSetup();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Session")
	void ToggleRuntimeDebugPanel();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Session")
	void ShowRuntimeDebugPanel();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Session")
	void HideRuntimeDebugPanel();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Stage")
	bool SelectStage(int32 StageIndex);

	UFUNCTION(BlueprintPure, Category = "Encounter Debug|Stage")
	int32 GetSelectedStageIndex() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Stage")
	bool TeleportPlayerToSelectedStage();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	AGP_EnemyCharacter* SpawnSelectedStageMob();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	AGP_EnemyCharacter* SpawnSelectedStageBoss();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	AGP_EnemyCharacter* SpawnSelectedClassMob();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	AGP_EnemyCharacter* SpawnSelectedClassBoss();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	AGP_EnemyCharacter* SpawnSelectedClass();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	TArray<FString> GetAvailableMobClassNames() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	bool SelectAvailableMobClass(int32 ClassIndex);

	UFUNCTION(BlueprintPure, Category = "Encounter Debug|Spawn")
	int32 GetSelectedAvailableMobClassIndex() const;

	UFUNCTION(BlueprintPure, Category = "Encounter Debug|Spawn")
	FString GetSelectedAvailableMobClassName() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	void SpawnSelectedStageMobAndBoss();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	int32 DespawnSelectedStageDebugEnemies();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Spawn")
	int32 DespawnAllDebugEnemies();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	TArray<FGPEncounterDebugEnemySnapshot> RefreshEnemyList();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	bool SelectEnemy(AGP_EnemyCharacter* Enemy);

	UFUNCTION(BlueprintPure, Category = "Encounter Debug|Enemy")
	AGP_EnemyCharacter* GetSelectedEnemy() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	FString GetSelectedEnemyDebugReport(EGPEncounterDebugReportMode ReportMode = EGPEncounterDebugReportMode::Summary) const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	FString GetSelectedEnemyHeaderReport() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	FString GetSelectedEnemySkillReport() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	FString GetSelectedEnemyLinkedActorReport() const;

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	bool KillSelectedEnemy();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	bool DestroySelectedEnemy();

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	bool SetSelectedEnemyHealthPercent(float HealthPercent);

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|Enemy")
	bool SetSelectedEnemyAIEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|GAS")
	bool AddLooseTagToSelectedEnemy(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|GAS")
	bool RemoveLooseTagFromSelectedEnemy(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|GAS")
	bool ToggleLooseTagOnSelectedEnemy(FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = "Encounter Debug|GAS")
	bool ApplyGameplayEffectToSelectedEnemy(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	UPROPERTY(BlueprintAssignable, Category = "Encounter Debug")
	FGPEncounterDebugLogMessage OnLogMessage;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Stage")
	TArray<FGPEncounterDebugStageSlot> StageSlots;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	float BossForwardOffset = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	float SpawnVerticalOffset = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	bool bDestroyExistingStageDebugEnemiesBeforeSpawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	TArray<TSubclassOf<AGP_EnemyCharacter>> AvailableMobClasses;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	bool bAutoDiscoverAvailableMobClasses = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	FName MobClassAssetRootPath = TEXT("/Game/Characters/EnemyCharacter");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	bool bSpawnAIEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	bool bFaceSpawnedEnemyToPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Spawn")
	bool bSelectSpawnedEnemy = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Tags")
	FName DebugSpawnedActorTag = TEXT("EncounterDebugSpawned");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Runtime UI")
	bool bEnableRuntimeDebugHotkey = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Encounter Debug|Runtime UI")
	FKey RuntimeDebugToggleKey = EKeys::F9;

private:
	UPROPERTY(Transient)
	TObjectPtr<UGP_EncounterDebugRuntimeWidget> RuntimeDebugWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<AGP_EnemyCharacter>> DebugSpawnedEnemies;

	UPROPERTY(Transient)
	TWeakObjectPtr<AGP_EnemyCharacter> SelectedEnemy;

	UPROPERTY(Transient)
	int32 SelectedStageIndex = 0;

	UPROPERTY(Transient)
	int32 SelectedAvailableMobClassIndex = 0;

	UPROPERTY(Transient)
	TArray<FSoftClassPath> DiscoveredMobClassPaths;

	UPROPERTY(Transient)
	TArray<FString> DiscoveredMobClassNames;

	AGP_EncounterDebugDirector* ResolveRuntimeDirectorForCall() const;
	AGP_PlayerCharacter* FindPlayerCharacter() const;
	bool ResolveStageTransform(int32 StageIndex, FTransform& OutTransform) const;
	AGP_EnemyCharacter* SpawnEnemyForSelectedStage(EGPEncounterDebugSpawnKind SpawnKind, TSubclassOf<AGP_EnemyCharacter> OverrideClass = nullptr);
	int32 DespawnSelectedStageDebugEnemiesOfKind(EGPEncounterDebugSpawnKind SpawnKind);
	TSubclassOf<AGP_EnemyCharacter> ResolveSelectedClassForSpawn() const;
	void EnsureAvailableMobClassList();
	int32 DestroyMatadorDecoysForBoss(const AActor* BossActor);
	void TrackDebugEnemy(AGP_EnemyCharacter* Enemy, int32 StageIndex, EGPEncounterDebugSpawnKind SpawnKind);
	bool IsDebugSpawnedEnemy(const AGP_EnemyCharacter* Enemy) const;
	int32 ResolveDebugStageIndex(const AGP_EnemyCharacter* Enemy) const;
	FGPEncounterDebugEnemySnapshot BuildEnemySnapshot(AGP_EnemyCharacter* Enemy) const;
	FString BuildEnemyDebugReport(AGP_EnemyCharacter* Enemy, EGPEncounterDebugReportMode ReportMode) const;
	FString BuildEnemyHeaderReport(AGP_EnemyCharacter* Enemy) const;
	FString BuildEnemySkillReport(AGP_EnemyCharacter* Enemy) const;
	FString BuildEnemyLinkedActorReport(AGP_EnemyCharacter* Enemy) const;
	void Log(const FString& Message) const;
};
