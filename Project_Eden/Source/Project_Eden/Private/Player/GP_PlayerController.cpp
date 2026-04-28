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
	

	UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);

	check(MoveAction);
	check(LookAction);
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Move);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_Look);
	
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Input_Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::Input_StopJump);
	}

	// --- [상태 전환 ] ---
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::Input_SprintPressed);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_SprintReleased);
	if (DashAction)   EnhancedInputComponent->BindAction(DashAction,   ETriggerEvent::Started, this, &ThisClass::Input_Dash);

	// --- [어빌리티 및 스킬] ---
	if (PrimaryAttackAction)  EnhancedInputComponent->BindAction(PrimaryAttackAction,  ETriggerEvent::Started,   this, &ThisClass::Input_PrimaryAttack);
	if (SkillSlot1Action) EnhancedInputComponent->BindAction(SkillSlot1Action, ETriggerEvent::Triggered, this, &ThisClass::Input_SkillSlot1);
	if (SkillSlot2Action) EnhancedInputComponent->BindAction(SkillSlot2Action, ETriggerEvent::Triggered, this, &ThisClass::Input_SkillSlot2);
	if (UltimateAction) EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Triggered, this, &ThisClass::Input_UltimateSkill);
	
}


void AGP_PlayerController::Input_Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;

	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void AGP_PlayerController::Input_Look(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AGP_PlayerController::Input_Jump()
{
	if (!IsValid(GetCharacter())) return;
	GetCharacter()->Jump();
}

void AGP_PlayerController::Input_StopJump()
{
	if (!IsValid(GetCharacter())) return;
	GetCharacter()->StopJumping();
}


void AGP_PlayerController::Input_ToggleSprint()
{
	if (auto* PC = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		PC->ToggleSprinting();
	}
}

void AGP_PlayerController::Input_SprintPressed()
{
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		if (bIsSprintToggle) PC->ToggleSprinting();
		else PC->StartSprinting();
	}
}

void AGP_PlayerController::Input_SprintReleased()
{
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		if (!bIsSprintToggle) PC->StopSprinting();
	}
}
void AGP_PlayerController::Input_Dash()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{

		if (PlayerCharacter->TryPerformDash())
		{
			return;
		}
	}

	ActivateAbilityByTag(GPTags::Ability::Movement::Dash);
}


void AGP_PlayerController::Input_PrimaryAttack()
{
	// [Rule: Early Return]
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn);
	if (!ASI || !ASI->GetAbilitySystemComponent()) return;
	
	FGameplayTag PrimaryTag = GPTags::Ability::Skill::Primary;
    
	// ASC를 통해 어빌리티 실행 (캐릭터의 멤버 함수 직접 호출 금지)
	ASI->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(FGameplayTagContainer(PrimaryTag));
}

void AGP_PlayerController::Input_SkillSlot1()
{
	UE_LOG(LogTemp, Warning, TEXT("Targeting"));
	ActivateAbilityByTag(GPTags::Ability::Skill::Slot01);
}

void AGP_PlayerController::Input_SkillSlot2()
{
	ActivateAbilityByTag(GPTags::Ability::Skill::Slot02);
}

void AGP_PlayerController::Input_UltimateSkill()
{
	ActivateAbilityByTag(GPTags::Ability::Skill::Ultimate);
}


bool AGP_PlayerController::ActivateAbilityByTag(const FGameplayTag& AbilityTag) const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return false;

	return ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
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
