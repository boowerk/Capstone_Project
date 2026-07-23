#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Game/GP_MiddleTravelTypes.h"
#include "GameplayTagContainer.h"
#include "GP_PlayerController.generated.h"

class AGP_PlayerCharacter;
class UAbilitySystemComponent;
class AGP_EnemyCharacter;
class UGP_AugmentSelectWidget;
class UGP_CharacterStatsMenuWidget;
class UGP_SkillAugmentData;
class UGP_SkillAugmentPoolData;
class UGP_SkillData;
class UGP_SkillPoolData;
class UGP_SkillSelectWidget;
class UGP_TestSkillSet;
class UGP_PlayerHUDWidget;
class UGP_MiddleTravelMapWidget;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
struct FGameplayTag;
struct FGameplayEventData;
struct FInputActionValue;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGP_PlayerController();
	FVector2D GetCurrentMoveInput() const { return CurrentMoveInput; }
	bool ShouldResumeHeldSprint() const { return !bIsSprintToggle && bSprintInputHeld; }
	bool IsCrouchInputHeld() const { return bCrouchInputHeld; }

	UFUNCTION(BlueprintCallable, Category = "UI|Augment")
	bool RequestOpenAugmentSelect();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "UI|Augment")
	void ClientOpenRegionEventAugmentSelect();

	UFUNCTION(BlueprintCallable, Category = "UI|Augment")
	bool OpenAugmentSelectWidget(const TArray<UGP_SkillAugmentData*>& Candidates);

	UFUNCTION(BlueprintCallable, Category = "UI|Augment")
	void CloseAugmentSelectWidget();

	UFUNCTION(BlueprintCallable, Category = "UI|Skill")
	bool RequestEquipSkill(UGP_SkillData* SkillData, FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category = "UI|Skill")
	void CloseSkillSelect();

	UFUNCTION(BlueprintPure, Category = "UI|Skill")
	UGP_SkillPoolData* GetSkillPoolData() const { return SkillPoolData; }

	UFUNCTION(Client, Reliable, Category = "UI|Middle Travel")
	void ClientOpenMiddleTravelMap(
		const TArray<FGPMiddleTravelDestination>& Destinations);

	UFUNCTION(Client, Reliable, Category = "UI|Middle Travel")
	void ClientConfirmMiddleTravel();

	UFUNCTION(BlueprintCallable, Category = "UI|Middle Travel")
	void RequestMiddleTravel(FName DestinationZoneId);

	UFUNCTION(BlueprintCallable, Category = "UI|Middle Travel")
	void CloseMiddleTravelMap();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

private:
	FVector2D CurrentMoveInput = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	// --- Movement 관련 ---
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> CrouchAction;

	// --- Abilities 관련 ---
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAttackAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> SecondarySkillConfirmAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> CancelSkillSelectionAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Input|Settings", meta = (AllowPrivateAccess = "true"))
	bool bIsSprintToggle = false;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> DashAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> SkillSlot1Action;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> SkillSlot2Action;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TObjectPtr<UInputAction> TestToggleSkillAction;

	// Test utility inputs are kept separate so skill preset rotation and White Void toggling can coexist.
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TObjectPtr<UInputAction> RotateTestSkillAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TObjectPtr<UGP_TestSkillSet> TestSkillSet;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TObjectPtr<UInputAction> WhiteVoidToggleAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TSubclassOf<class UGameplayAbility> WaterPuddleAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TSubclassOf<class UGameplayAbility> NetTestProjectileAbilityClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGP_PlayerHUDWidget> HUDWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Middle Travel")
	TSubclassOf<UGP_MiddleTravelMapWidget> MiddleTravelMapWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGP_MiddleTravelMapWidget> MiddleTravelMapWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Character Menu")
	TSubclassOf<UGP_CharacterStatsMenuWidget> CharacterStatsMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|UI")
	TObjectPtr<UInputAction> CharacterStatsMenuAction;

	UPROPERTY(Transient)
	TObjectPtr<UGP_CharacterStatsMenuWidget> CharacterStatsMenuWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Skill")
	TSubclassOf<UGP_SkillSelectWidget> SkillSelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|UI")
	TObjectPtr<UInputAction> OpenSkillSelectAction;

	UPROPERTY(Transient)
	TObjectPtr<UGP_SkillSelectWidget> SkillSelectWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	TSubclassOf<UGP_AugmentSelectWidget> AugmentSelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Augment")
	TObjectPtr<UGP_SkillAugmentPoolData> AugmentPoolData;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|Augment", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 AugmentCandidateCount = 3;

	UPROPERTY(Transient)
	TObjectPtr<UGP_AugmentSelectWidget> AugmentSelectWidget;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Skill")
	TObjectPtr<UGP_SkillPoolData> SkillPoolData;

	UPROPERTY(Transient)
	TObjectPtr<AGP_EnemyCharacter> CurrentBossEnemy;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Boss")
	float BossDetectionRange = 3000.0f;

	UPROPERTY(EditDefaultsOnly, Category = "UI|Boss")
	float BossRefreshInterval = 0.2f;

	float BossRefreshAccumulator = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Movement|Smoothing", meta = (ClampMin = "0.0"))
	float MaxWalkSpeedInterpSpeed = 1200.0f;

	float TargetMaxWalkSpeed = 0.0f;
	bool bHasTargetMaxWalkSpeed = false;
	bool bSprintInputHeld = false;
	bool bCrouchInputHeld = false;
	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump();
	void Input_StopJump();
	void Input_CrouchPressed();
	void Input_CrouchReleased();

	// --- 상태 제어 (State) ---
	void Input_ToggleSprint();
	void Input_SprintPressed();
	void Input_SprintReleased();
	void Input_Dash();

	// --- 전투 및 스킬 (Combat) ---
	void Input_PrimaryAttack();
	void Input_SecondarySkillConfirm();
	void Input_CancelSkillSelection();
	void Input_SkillSlot1();
	void Input_SkillSlot2();
	void Input_UltimateSkill();
	void Input_SkillSlotReleased();
	void Input_TestToggleSkill();
	void Input_RotateTestSkill();
	void Input_ToggleWhiteVoid();
	void Input_ToggleCharacterStatsMenu();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TestToggleSkill();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RotateTestSkill();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendSkillSelectionEvent(FGameplayTag EventTag, FVector_NetQuantize TargetLocation, bool bHasTargetLocation);

	UFUNCTION(Server, Unreliable, WithValidation)
	void Server_UpdateMoveInput(FVector2D MovementVector);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_ClearMoveInput();

	UFUNCTION(Server, Reliable)
	void Server_EquipSkill(UGP_SkillData* SkillData, FGameplayTag SlotTag);

	UFUNCTION(Server, Reliable)
	void Server_RequestMiddleTravel(FName DestinationZoneId);

	bool CanEquipSkill(UGP_SkillData* SkillData, FGameplayTag SlotTag) const;
	bool BuildSkillLoadoutAfterEquip(
		UGP_SkillData* SkillData,
		FGameplayTag SlotTag,
		UGP_SkillData*& OutSlot01Skill,
		UGP_SkillData*& OutSlot02Skill) const;
	bool ActivateAbilityByTag(const FGameplayTag& AbilityTag) const;
	bool IsSkillSelectionActive() const;
	bool IsGroundPositionSelectionActive() const;
	bool SendSkillSelectionEvent(const FGameplayTag& EventTag) const;
	void FillSkillSelectionTargetData(FGameplayEventData& Payload, const FVector& TargetLocation) const;
	bool GetSkillSelectionCursorLocation(FVector& OutTargetLocation) const;
	void UpdateSkillSelectionInputMode();
	void CancelSkillSelectionIfActive() const;
	void ClearTestSkillSlots(UAbilitySystemComponent* ASC);
	int32 GetTestSkillPresetCount() const;
	void EquipTestSkillPreset(AGP_PlayerCharacter* PlayerCharacter, int32 PresetIndex);
	void RefreshBossHUD();
	bool EnsureCharacterStatsMenuWidget();
	void OpenCharacterStatsMenu();
	void CloseCharacterStatsMenu();
	void ApplyCharacterStatsMenuInputMode(bool bMenuOpen);
	void Input_ToggleSkillSelect();
	bool EnsureSkillSelectWidget();
	void OpenSkillSelect();
	void ApplySkillSelectInputMode(bool bMenuOpen);
	bool EnsureMiddleTravelMapWidget();
	void ApplyMiddleTravelInputMode(bool bMenuOpen);
	FVector2D ResolveEffectiveMoveInput(const AGP_PlayerCharacter* PlayerCharacter) const;
	void UpdateMovementSpeed(float DeltaSeconds);
	void UpdateCharacterRotation(float DeltaSeconds);

	bool bSkillsEquipped = false;
	int32 TestSkillPresetIndex = 0;
	bool bIsCharacterStatsMenuOpen = false;
	bool bWasGroundPositionSelectionActive = false;
	bool bSkillSelectInputBlocked = false;

	// --- Movement Input Smoothing ---
	FVector SmoothedMoveWorldDirection = FVector::ZeroVector;
	bool bHasSmoothedMoveWorldDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Smoothing", meta = (AllowPrivateAccess = "true"))
	float MoveDirectionInterpSpeed = 12.0f;

	bool ShouldSmoothMoveDirection() const;
	void ResetMoveDirectionSmoothing();
	void Input_MoveCompleted(const FInputActionValue& Value);

	// Adds the configured Input Mapping Contexts to the local player's Enhanced
	// Input subsystem. Safe to call multiple times; no-op on non-local or when
	// the subsystem is not yet available (e.g. early during dedicated-server
	// client travel). Called from both SetupInputComponent and BeginPlay so a
	// null subsystem at input-setup time on clients does not leave input unbound.
	void AddInputMappingContexts();
};
