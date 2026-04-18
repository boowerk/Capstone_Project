#include "Player/GP_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameplayTags/GP_Tags.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "UI/GP_PlayerHUDWidget.h"

AGP_PlayerController::AGP_PlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AGP_PlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (!IsLocalController() || HUDWidget)
	{
		return;
	}

	UClass* WidgetClass = HUDWidgetClass ? HUDWidgetClass.Get() : nullptr;
	if (!IsValid(WidgetClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("HUDWidgetClass is not set on %s. Assign a Widget Blueprint based on GP_PlayerHUDWidget."), *GetName());
		return;
	}

	UUserWidget* CreatedWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (!IsValid(CreatedWidget))
	{
		return;
	}

	CreatedWidget->AddToViewport();
	HUDWidget = Cast<UGP_PlayerHUDWidget>(CreatedWidget);
	if (HUDWidget)
	{
		HUDWidget->SetBossVisible(false);
	}
}

void AGP_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	BossRefreshAccumulator += DeltaSeconds;
	if (BossRefreshAccumulator >= BossRefreshInterval)
	{
		BossRefreshAccumulator = 0.0f;
		RefreshBossHUD();
	}
}

void AGP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;

	for (UInputMappingContext* Context : InputMappingContexts)
	{
		InputSubsystem->AddMappingContext(Context, 0);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;

	if (IsValid(MoveAction))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	}
	if (IsValid(JumpAction))
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
	}
	if (IsValid(LookAction))
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
	}
	if (IsValid(PrimaryAction))
	{
		EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Triggered, this, &ThisClass::Primary);
	}

	if (IsValid(Skill_Q_Action))
	{
		EnhancedInputComponent->BindAction(Skill_Q_Action, ETriggerEvent::Triggered, this, &ThisClass::Skill_Q);
	}
	else if (IsValid(TargetingAction))
	{
		EnhancedInputComponent->BindAction(TargetingAction, ETriggerEvent::Started, this, &ThisClass::Targeting);
	}

	if (IsValid(Skill_E_Action))
	{
		EnhancedInputComponent->BindAction(Skill_E_Action, ETriggerEvent::Triggered, this, &ThisClass::Skill_E);
	}
	else if (IsValid(SkillAction))
	{
		EnhancedInputComponent->BindAction(SkillAction, ETriggerEvent::Triggered, this, &ThisClass::Skill);
	}

	if (IsValid(Skill_R_Action))
	{
		EnhancedInputComponent->BindAction(Skill_R_Action, ETriggerEvent::Triggered, this, &ThisClass::Skill_R);
	}
	else if (IsValid(UltimateAction))
	{
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Triggered, this, &ThisClass::Ultimate);
	}

	if (IsValid(DashAction))
	{
		EnhancedInputComponent->BindAction(DashAction, ETriggerEvent::Started, this, &ThisClass::Dash);
	}
	else if (IsValid(InputComponent))
	{
		// Fallback roll key until an enhanced input action asset is assigned.
		InputComponent->BindKey(EKeys::LeftAlt, IE_Pressed, this, &ThisClass::Dash);
	}

	if (IsValid(InputComponent))
	{
		// Shift 입력은 별도 액션 에셋 없이도 항상 걷기/달리기 전환이 되도록 직접 바인딩한다.
		InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &ThisClass::StartSprint);
		InputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &ThisClass::StopSprint);
		InputComponent->BindKey(EKeys::RightShift, IE_Pressed, this, &ThisClass::StartSprint);
		InputComponent->BindKey(EKeys::RightShift, IE_Released, this, &ThisClass::StopSprint);
	}
}

void AGP_PlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	if (const AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		if (PlayerCharacter->IsRolling() || PlayerCharacter->IsPrimaryAttacking() || PlayerCharacter->IsSprintExitControlLocked())
		{
			return;
		}
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void AGP_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AGP_PlayerController::Jump()
{
	if (!IsValid(GetCharacter())) return;
	if (const AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		if (PlayerCharacter->IsRolling() || PlayerCharacter->IsPrimaryAttacking() || PlayerCharacter->IsSprintExitControlLocked())
		{
			return;
		}
	}

	GetCharacter()->Jump();
}

void AGP_PlayerController::StopJump()
{
	if (!IsValid(GetCharacter())) return;
	GetCharacter()->StopJumping();
}

void AGP_PlayerController::StartSprint()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		if (PlayerCharacter->IsSprintExitControlLocked())
		{
			return;
		}

		PlayerCharacter->SetSprinting(true);
	}
}

void AGP_PlayerController::StopSprint()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		PlayerCharacter->SetSprinting(false);
	}
}

void AGP_PlayerController::Primary()
{
	ActivateAbilityByTag(GPTags::GPAbilities::Primary);
}

void AGP_PlayerController::Targeting()
{
	UE_LOG(LogTemp, Warning, TEXT("Targeting"));
	ActivateAbilityByTag(GPTags::GPAbilities::Targeting);
}

void AGP_PlayerController::Skill()
{
	ActivateAbilityByTag(GPTags::GPAbilities::Skill);
}

void AGP_PlayerController::Ultimate()
{
	ActivateAbilityByTag(GPTags::GPAbilities::Ultimate);
}

void AGP_PlayerController::Skill_Q()
{
	UE_LOG(LogTemp, Warning, TEXT("Targeting"));
	ActivateAbilityByTag(GPTags::GPAbilities::Skill_Q);
}

void AGP_PlayerController::Skill_E()
{
	ActivateAbilityByTag(GPTags::GPAbilities::Skill_E);
}

void AGP_PlayerController::Skill_R()
{
	ActivateAbilityByTag(GPTags::GPAbilities::Skill_R);
}

bool AGP_PlayerController::ActivateAbilityByTag(const FGameplayTag& AbilityTag) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return false;

	return ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

void AGP_PlayerController::Dash()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		if (PlayerCharacter->IsSprintExitControlLocked())
		{
			return;
		}

		if (PlayerCharacter->TryPerformRoll())
		{
			return;
		}
	}

	ActivateAbilityByTag(GPTags::GPAbilities::Dash);
}

void AGP_PlayerController::RefreshBossHUD()
{
	if (!HUDWidget || !GetPawn())
	{
		return;
	}

	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(this, AGP_EnemyCharacter::StaticClass(), EnemyActors);

	AGP_EnemyCharacter* ClosestBoss = nullptr;
	float ClosestDistanceSq = BossDetectionRange * BossDetectionRange;
	const FVector PlayerLocation = GetPawn()->GetActorLocation();

	for (AActor* Actor : EnemyActors)
	{
		AGP_EnemyCharacter* EnemyCharacter = Cast<AGP_EnemyCharacter>(Actor);
		if (!EnemyCharacter || !EnemyCharacter->IsBossEnemy())
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(PlayerLocation, EnemyCharacter->GetActorLocation());
		if (DistanceSq <= ClosestDistanceSq)
		{
			ClosestDistanceSq = DistanceSq;
			ClosestBoss = EnemyCharacter;
		}
	}

	if (!ClosestBoss)
	{
		CurrentBossEnemy = nullptr;
		HUDWidget->SetBossVisible(false);
		return;
	}

	CurrentBossEnemy = ClosestBoss;
	HUDWidget->SetBossText(CurrentBossEnemy->GetBossDisplayName());
	HUDWidget->SetBossVisible(true);
}
