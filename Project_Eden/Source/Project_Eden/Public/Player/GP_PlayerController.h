#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GP_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class UGP_PlayerHUDWidget;
class AGP_EnemyCharacter;
struct FInputActionValue;
struct FGameplayTag;
UCLASS()
class PROJECT_EDEN_API AGP_PlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AGP_PlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupInputComponent() override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;
	
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;


	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> TargetingAction;


	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> SkillAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Abilities")
	TObjectPtr<UInputAction> UltimateAction;

	UPROPERTY(EditDefaultsOnly, Category = "GAS|Input|Movement")
	TObjectPtr<UInputAction> DashAction;

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

	
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump();
	void StopJump();

	void Primary();
	void ActivateAbilityByTag(const FGameplayTag& AbilityTag) const;
	void Skill();
	void Ultimate();
	void Dash();

	void Targeting();
	void RefreshBossHUD();
	
};
