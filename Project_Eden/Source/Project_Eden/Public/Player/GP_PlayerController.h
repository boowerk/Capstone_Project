#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GP_PlayerController.generated.h"

class AGP_EnemyCharacter;
class UGP_PlayerHUDWidget;
class UInputAction;
class UInputMappingContext;
class UUserWidget;
struct FGameplayTag;
struct FInputActionValue;

UCLASS()
class PROJECT_EDEN_API AGP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGP_PlayerController();
	FVector2D GetCurrentMoveInput() const { return CurrentMoveInput; }

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

	// --- Abilities 관련 ---
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAttackAction;

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

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Tests")
	TSubclassOf<class UGameplayAbility> WaterPuddleAbilityClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UGP_PlayerHUDWidget> HUDWidget;

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

	void Input_Move(const FInputActionValue& Value);
	void Input_Look(const FInputActionValue& Value);
	void Input_Jump();
	void Input_StopJump();

	// --- 상태 제어 (State) ---
	void Input_ToggleSprint();
	void Input_SprintPressed();
	void Input_SprintReleased();
	void Input_Dash();

	// --- 전투 및 스킬 (Combat) ---
	void Input_PrimaryAttack();
	void Input_SkillSlot1();
	void Input_SkillSlot2();
	void Input_UltimateSkill();
	void Input_TestToggleSkill();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_TestToggleSkill();

	bool ActivateAbilityByTag(const FGameplayTag& AbilityTag) const;
	void RefreshBossHUD();
	void UpdateMovementSpeed(float DeltaSeconds);
	void UpdateCharacterRotation(float DeltaSeconds);

	bool bSkillsEquipped = false;

private:
	// --- Movement Input Smoothing ---
	FVector SmoothedMoveWorldDirection = FVector::ZeroVector;
	bool bHasSmoothedMoveWorldDirection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Smoothing", meta = (AllowPrivateAccess = "true"))
	float MoveDirectionInterpSpeed = 12.0f;

	bool ShouldSmoothMoveDirection() const;
	void ResetMoveDirectionSmoothing();
	void Input_MoveCompleted(const FInputActionValue& Value);
};
