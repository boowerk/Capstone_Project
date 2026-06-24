#pragma once

#include "CoreMinimal.h"
#include "AI/Data/EnemyArchetypeData.h"
#include "Characters/GP_BaseCharacter.h"
#include "GameplayTagContainer.h"
#include "GP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UBehaviorTree;
class UBlackboardData;
class UEnemyAIRangeVisualizationComponent;
class UEnemyArchetypeData;
class UGP_BossTargetMarkerVFXComponent;
class UGP_EnemyDeathAbility;
class UGP_WidgetComponent;
class UPDA_EnemyAnimationSet;
class AGP_EnemyCharacter;
class AGP_PlayerState;
struct FOnAttributeChangeData;
struct FDataTableRowHandle;
struct FEnemyArchetypeTuning;
struct FEnemyLLMEvaluation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEnemyDeathStarted, AGP_EnemyCharacter*, Enemy, AActor*, InstigatorActor);

UENUM(BlueprintType)
enum class EGPEnemyCombatArchetype : uint8
{
	Melee UMETA(DisplayName = "Melee"),
	Ranged UMETA(DisplayName = "Ranged"),
	Flying UMETA(DisplayName = "Flying")
};

// Broadcast on the server the first time this enemy's health reaches zero, so the GameMode can track zone clears.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGPOnEnemyDied, AGP_EnemyCharacter*, DeadEnemy);

UCLASS()
class PROJECT_EDEN_API AGP_EnemyCharacter : public AGP_BaseCharacter
{
	GENERATED_BODY()

public:
	AGP_EnemyCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void UpdateAnimationSet() override;

	UPDA_EnemyAnimationSet* GetEnemyAnimationSet() const { return EnemyAnimationSet; }

	UFUNCTION(BlueprintPure, Category = "Boss")
	bool IsBossEnemy() const { return bIsBossEnemy; }

	// Fires once (server authority) when this enemy dies. The GameMode subscribes to count down zone clears.
	UPROPERTY(BlueprintAssignable, Category = "Enemy|Lifecycle")
	FGPOnEnemyDied OnEnemyDied;

	UFUNCTION(BlueprintPure, Category = "Boss")
	FText GetBossDisplayName() const;

	UFUNCTION(BlueprintPure, Category = "Boss|VFX")
	UGP_BossTargetMarkerVFXComponent* GetBossTargetMarkerVFXComponent() const { return BossTargetMarkerVFXComponent; }

	// AIController calls this only when TargetActor is first acquired or changes to another valid player.
	void NotifyBossTargetSelected(AActor* TargetActor);

	UFUNCTION(BlueprintPure, Category = "AI")
	FVector GetBehaviorAnchorLocation() const;

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetReturnHomeDistance() const { return FMath::Max(0.0f, ReturnHomeDistance); }

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetReturnHomeAcceptanceRadius() const { return FMath::Max(0.0f, ReturnHomeAcceptanceRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Ranges")
	float GetPatrolRadius() const { return FMath::Max(0.0f, PatrolRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetSightRadius() const { return FMath::Max(0.0f, SightRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetLoseSightRadius() const { return FMath::Max(GetSightRadius(), LoseSightRadius); }

	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	float GetPeripheralVisionAngleDegrees() const { return FMath::Clamp(PeripheralVisionAngleDegrees, 0.0f, 180.0f); }

	UFUNCTION(BlueprintPure, Category = "AI|Archetype")
	EGPEnemyCombatArchetype GetCombatArchetype() const { return CombatArchetype; }

	UFUNCTION(BlueprintPure, Category = "AI|Combat")
	FGameplayTag GetDefaultAttackAbilityTag() const { return DefaultAttackAbilityTag; }

	UFUNCTION(BlueprintPure, Category = "Enemy|Death")
	bool IsDead() const { return bIsDead; }

	UFUNCTION(BlueprintPure, Category = "Enemy|UI")
	UGP_WidgetComponent* GetWorldHealthBarComponent() const { return WorldHealthBarComponent; }

	// Public authority entry point also supports scripted kills while zero-health deaths arrive through AttributeSet.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Enemy|Death")
	void RequestDeath(AActor* InstigatorActor);

	UPROPERTY(BlueprintAssignable, Category = "Enemy|Death")
	FOnEnemyDeathStarted OnEnemyDeathStarted;

	UFUNCTION(BlueprintPure, Category = "AI")
	UBehaviorTree* GetBehaviorTreeAssetOverride() const { return BehaviorTreeAssetOverride; }

	UFUNCTION(BlueprintPure, Category = "AI")
	UBlackboardData* GetBlackboardAssetOverride() const { return BlackboardAssetOverride; }

	// 현재 적 인스턴스의 시작 성향값을 계산해 공유 Blackboard에 넣을 준비를 한다.
	bool BuildInitialEnemyEvaluation(FEnemyLLMEvaluation& OutEvaluation) const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Enemy-only visual and combat animation data. Leave the legacy base AnimationSet empty for new enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UPDA_EnemyAnimationSet> EnemyAnimationSet;

	// 향후 EQS나 복귀 로직에서 사용할 기준 위치를 월드에 배치할 수 있도록 유지한다.
	UPROPERTY(EditInstanceOnly, Category = "AI", meta = (MakeEditWidget = "true"))
	FVector BehaviorAnchorOffset = FVector::ZeroVector;

	// Designers tune these on the placed enemy; BT, EQS, perception, and editor debug all read the same values.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float ReturnHomeDistance = 2200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float ReturnHomeAcceptanceRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Ranges", meta = (ClampMin = "0.0", Units = "cm"))
	float PatrolRadius = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float SightRadius = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", Units = "cm"))
	float LoseSightRadius = 2400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "deg"))
	float PeripheralVisionAngleDegrees = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Archetype")
	EGPEnemyCombatArchetype CombatArchetype = EGPEnemyCombatArchetype::Melee;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Debug")
	bool bShowAIRangesInEditor = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Debug", meta = (EditCondition = "bShowAIRangesInEditor"))
	bool bDrawAIRangesOnlyWhenSelected = true;

	// 테스트 중에는 적 액터 자체에서 공유 BT/Blackboard를 직접 지정할 수 있게 노출한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Assets")
	TObjectPtr<UBehaviorTree> BehaviorTreeAssetOverride;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Assets")
	TObjectPtr<UBlackboardData> BlackboardAssetOverride;

	// 가장 권장하는 경로로, 적 아키타입별 기본 성향을 DataAsset으로 정의한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	TObjectPtr<UEnemyArchetypeData> EnemyArchetypeData;

	// 필요하면 DataTable RowHandle로도 같은 구조를 읽을 수 있게 열어 둔다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config", meta = (RowType = "/Script/Project_Eden.EnemyArchetypeTableRow"))
	FDataTableRowHandle EnemyArchetypeRow;

	// Built-in presets make inherited C++ enemy parents usable before a designer creates a dedicated DataAsset.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	bool bUseBuiltInArchetypeTuning = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config", meta = (EditCondition = "bUseBuiltInArchetypeTuning"))
	FEnemyArchetypeTuning BuiltInArchetypeTuning;

	// 디버깅이나 고정 개성 연출이 필요할 때 랜덤 시드를 수동 고정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config")
	bool bOverridePersonalitySeed = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Config", meta = (EditCondition = "bOverridePersonalitySeed"))
	int32 PersonalitySeedOverride = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	bool bIsBossEnemy = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss", meta = (EditCondition = "bIsBossEnemy"))
	FText BossDisplayName;

	// Boss 플래그만 켜도 기본 강공격/광역/휩쓸기 어빌리티를 받을 수 있게 합니다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss|Abilities", meta = (EditCondition = "bIsBossEnemy"))
	bool bGrantDefaultBossPatternAbilities = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Abilities")
	bool bGrantDefaultEnemyAttackAbility = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Abilities", meta = (EditCondition = "bGrantDefaultEnemyAttackAbility"))
	TSubclassOf<UGameplayAbility> DefaultEnemyAttackAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Abilities", meta = (EditCondition = "bGrantDefaultEnemyAttackAbility"))
	FGameplayTag DefaultAttackAbilityTag;

	// Every enemy receives one tag-addressable death ability unless StartupAbilities already supplies a custom replacement.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Abilities")
	bool bGrantDefaultEnemyDeathAbility = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Abilities", meta = (EditCondition = "bGrantDefaultEnemyDeathAbility"))
	TSubclassOf<UGameplayAbility> DefaultEnemyDeathAbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0"))
	float XPReward = 25.0f;

	// Regular enemies show this bar automatically; bosses keep using the dedicated HUD boss bar.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|UI")
	bool bShowWorldHealthBar = true;

	// Only show the world health bar when the local player is within this distance. 0 disables the distance limit.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|UI", meta = (EditCondition = "bShowWorldHealthBar", ClampMin = "0.0", Units = "cm"))
	float HealthBarVisibleDistance = 1500.0f;

	// How often each client re-evaluates the distance check. Cheaper than per-frame; the bar pops in/out at this cadence.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|UI", meta = (EditCondition = "bShowWorldHealthBar", ClampMin = "0.05", Units = "s"))
	float HealthBarDistanceCheckInterval = 0.25f;

	// No death montage is played yet; the actor remains visible in its current pose until this delay expires.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Enemy|Death", meta = (ClampMin = "0.0", Units = "s"))
	float DeathDespawnDelay = 2.0f;

	virtual void HandlePostDamageTaken(AActor* InstigatorActor, float DamageAmount, FGameplayTag ElementTag) override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Enemy|Death")
	void BP_OnDeathStarted(AActor* InstigatorActor);

private:
	friend class UGP_EnemyDeathAbility;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, Category = "AI|Debug", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEnemyAIRangeVisualizationComponent> AIRangeVisualizer;
#endif

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Boss|VFX", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_BossTargetMarkerVFXComponent> BossTargetMarkerVFXComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Enemy|UI", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_WidgetComponent> WorldHealthBarComponent;

	FVector BehaviorAnchorLocation = FVector::ZeroVector;
	bool bHasBehaviorAnchorLocation = false;
	bool bXPRewardGranted = false;
	bool bDeathRequested = false;

	UPROPERTY(ReplicatedUsing = OnRep_IsDead)
	bool bIsDead = false;

	UPROPERTY(Replicated)
	TObjectPtr<AActor> DeathInstigatorActor;

	bool bDeathStateApplied = false;

	const FEnemyArchetypeTuning* ResolveEnemyArchetypeTuning() const;
	int32 ResolvePersonalitySeed() const;
	void GiveDefaultEnemyAttackAbility();
	void GiveDefaultEnemyDeathAbility();
	void GiveDefaultBossPatternAbilities();

	UFUNCTION()
	void HandleOutOfHealth(AActor* InstigatorActor, AActor* TargetActor);

	UFUNCTION()
	void OnRep_IsDead();

	void EnterDeathStateFromAbility();
	void ApplyDeathState();
	void RefreshWorldHealthBarVisibility();
	void StartHealthBarDistanceCulling();
	bool IsLocalPlayerWithinHealthBarDistance() const;
	FTimerHandle HealthBarDistanceTimerHandle;
	void RefreshAIRangeVisualizers();
	void GrantXPRewardToInstigator(AActor* InstigatorActor);
	AGP_PlayerState* ResolveInstigatorPlayerState(AActor* InstigatorActor) const;
	void HandleMoveSpeedAttributeChanged(const FOnAttributeChangeData& ChangeData);
	void BindMoveSpeedAttribute();
	void UnbindMoveSpeedAttribute();

	FDelegateHandle MoveSpeedAttributeDelegateHandle;
};
