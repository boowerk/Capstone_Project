#include "Player/GP_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Abilities/GP_TestSkillSet.h"
#include "Blueprint/UserWidget.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
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

		// HUD 생성 시점에 이미 Pawn이 준비되어 있다면 즉시 바인딩 시도
		if (AGP_BaseCharacter* BaseChar = Cast<AGP_BaseCharacter>(GetPawn()))
		{
			if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
			{
				HUDWidget->BindToASC(ASC);
			}
		}
	}
}

void AGP_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovementSpeed(DeltaSeconds);
	UpdateCharacterRotation(DeltaSeconds);

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
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &ThisClass::Input_MoveCompleted);
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
	if (TestToggleSkillAction) EnhancedInputComponent->BindAction(TestToggleSkillAction, ETriggerEvent::Started, this, &ThisClass::Input_TestToggleSkill);
	// Keep both debug bindings after the PR merge so neither test preset rotation nor White Void input is dropped.
	if (RotateTestSkillAction) EnhancedInputComponent->BindAction(RotateTestSkillAction, ETriggerEvent::Started, this, &ThisClass::Input_RotateTestSkill);
	if (WhiteVoidToggleAction) EnhancedInputComponent->BindAction(WhiteVoidToggleAction, ETriggerEvent::Started, this, &ThisClass::Input_ToggleWhiteVoid);
}


void AGP_PlayerController::Input_Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;

	const FVector2D MovementVector = Value.Get<FVector2D>().GetClampedToMaxSize(1.f);
	CurrentMoveInput = MovementVector;

	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn());
	const bool bFixed = PlayerCharacter &&
		PlayerCharacter->GetAbilitySystemComponent() &&
		PlayerCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(GPTags::State::Status::Fixed);
	if (UCharacterMovementComponent* MoveComp = PlayerCharacter ? PlayerCharacter->GetCharacterMovement() : nullptr)
	{
		MoveComp->bOrientRotationToMovement = false;
		MoveComp->bUseControllerDesiredRotation = !MovementVector.IsNearlyZero() && !bFixed;
	}

	if (MovementVector.IsNearlyZero())
	{
		ResetMoveDirectionSmoothing();
		return;
	}

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const bool bSprinting = PlayerCharacter && PlayerCharacter->IsSprinting();
	if (PlayerCharacter)
	{
		TargetMaxWalkSpeed = PlayerCharacter->ResolveDirectionalMoveSpeed(MovementVector, bSprinting);
		bHasTargetMaxWalkSpeed = true;
	}

	FVector RawWorldDirection = (ForwardDirection * MovementVector.Y) + (RightDirection * MovementVector.X);
	RawWorldDirection = RawWorldDirection.GetSafeNormal();

	FVector FinalMoveDirection = RawWorldDirection;

	if (ShouldSmoothMoveDirection())
	{
		if (!bHasSmoothedMoveWorldDirection)
		{
			SmoothedMoveWorldDirection = RawWorldDirection;
			bHasSmoothedMoveWorldDirection = true;
			FinalMoveDirection = RawWorldDirection;
		}
		else
		{
			if (FVector::DotProduct(SmoothedMoveWorldDirection, RawWorldDirection) < 0.5f)
			{
				SmoothedMoveWorldDirection = RawWorldDirection;
			}
			else
			{
				SmoothedMoveWorldDirection = FMath::VInterpTo(
					SmoothedMoveWorldDirection,
					RawWorldDirection,
					GetWorld()->GetDeltaSeconds(),
					MoveDirectionInterpSpeed
				).GetSafeNormal();
			}
			FinalMoveDirection = SmoothedMoveWorldDirection;
		}
	}
	else
	{
		ResetMoveDirectionSmoothing();
		FinalMoveDirection = RawWorldDirection;
	}

	GetPawn()->AddMovementInput(FinalMoveDirection, MovementVector.Size());
}

void AGP_PlayerController::Input_MoveCompleted(const FInputActionValue& Value)
{
	CurrentMoveInput = FVector2D::ZeroVector;
	ResetMoveDirectionSmoothing();
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		if (UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement())
		{
			MoveComp->bUseControllerDesiredRotation = false;
		}
	}
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
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn)) return;

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn);
	if (!ASI || !ASI->GetAbilitySystemComponent()) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	FGameplayTag PrimaryTag = GPTags::Ability::Skill::Primary;

	bool bInputHandled = false;

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		// 부여된 동적 태그나 어빌리티 기본 태그 중 PrimaryTag가 있는지 검사
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(PrimaryTag) || 
		   (Spec.Ability && Spec.Ability->GetAssetTags().HasTagExact(PrimaryTag)))
		{
			if (Spec.IsActive())
			{
				UE_LOG(LogTemp, Warning, TEXT("Controller: Active Primary Ability Found. Sending Input."));
				ASC->AbilitySpecInputPressed(Spec);
				bInputHandled = true;
				break;
			}
		}
	}

	if (!bInputHandled)
	{
		UE_LOG(LogTemp, Warning, TEXT("Controller: Activating Primary Ability for the first time."));
		ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(PrimaryTag));
	}
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

	// 동적 태그(슬롯)를 가진 어빌리티를 먼저 활성화 시도
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(AbilityTag))
		{
			return ASC->TryActivateAbility(Spec.Handle);
		}
	}

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
		if (CurrentBossEnemy)
		{
			HUDWidget->ClearBossASC();
		}
		CurrentBossEnemy = nullptr;
		HUDWidget->SetBossVisible(false);
		return;
	}

	CurrentBossEnemy = ClosestBoss;
	HUDWidget->BindBossToASC(CurrentBossEnemy->GetAbilitySystemComponent());
	HUDWidget->SetBossText(CurrentBossEnemy->GetBossDisplayName());
	HUDWidget->SetBossVisible(true);
}

void AGP_PlayerController::UpdateMovementSpeed(float DeltaSeconds)
{
	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		bHasTargetMaxWalkSpeed = false;
		return;
	}

	const bool bSprinting = PlayerCharacter->IsSprinting();
	if (CurrentMoveInput.IsNearlyZero())
	{
		TargetMaxWalkSpeed = bSprinting ? PlayerCharacter->GetScaledSprintSpeed() : PlayerCharacter->GetScaledNormalWalkSpeed();
		bHasTargetMaxWalkSpeed = true;
	}
	else if (!bHasTargetMaxWalkSpeed)
	{
		TargetMaxWalkSpeed = PlayerCharacter->ResolveDirectionalMoveSpeed(CurrentMoveInput, bSprinting);
		bHasTargetMaxWalkSpeed = true;
	}

	if (UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement())
	{
		MoveComp->MaxWalkSpeed = FMath::FInterpConstantTo(
			MoveComp->MaxWalkSpeed,
			TargetMaxWalkSpeed,
			DeltaSeconds,
			MaxWalkSpeedInterpSpeed);
	}
}

void AGP_PlayerController::Input_TestToggleSkill()
{
	Server_TestToggleSkill();
}

void AGP_PlayerController::Input_RotateTestSkill()
{
	Server_RotateTestSkill();
}

void AGP_PlayerController::Input_ToggleWhiteVoid()
{
	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		PlayerCharacter->ToggleWhiteVoid();
	}
}

bool AGP_PlayerController::Server_TestToggleSkill_Validate()
{
	return true;
}

void AGP_PlayerController::Server_TestToggleSkill_Implementation()
{
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PC) return;

	UAbilitySystemComponent* ASC = PC->GetAbilitySystemComponent();
	if (!ASC) return;

	if (!bSkillsEquipped)
	{
		// 장착 로직
		if (WaterPuddleAbilityClass)
		{
			PC->EquipSkillByClass(GPTags::Ability::Skill::Slot01, WaterPuddleAbilityClass);
			PC->EquipSkillByClass(GPTags::Ability::Skill::Slot02, WaterPuddleAbilityClass);
			bSkillsEquipped = true;
			
			if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, TEXT("Skills Equipped to Slot 1 & 2!"));
			UE_LOG(LogTemp, Warning, TEXT("Server: WaterPuddle equipped to Slot 1 & 2"));
		}
	}
	else
	{
		// 해제 로직
		TArray<FGameplayTag> SlotTags = { GPTags::Ability::Skill::Slot01, GPTags::Ability::Skill::Slot02 };
		TArray<FGameplayAbilitySpecHandle> HandlesToRemove;
		
		for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
		{
			for (const FGameplayTag& SlotTag : SlotTags)
			{
				if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
				{
					HandlesToRemove.Add(Spec.Handle);
				}
			}
		}

		for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
		{
			ASC->ClearAbility(Handle);
		}

		bSkillsEquipped = false;
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Orange, TEXT("Skills Unequipped!"));
		UE_LOG(LogTemp, Warning, TEXT("Server: WaterPuddle unequipped from Slot 1 & 2"));
	}
}

bool AGP_PlayerController::Server_RotateTestSkill_Validate()
{
	return true;
}

void AGP_PlayerController::Server_RotateTestSkill_Implementation()
{
	// 여기서 프리셋 순환 장착
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PC) return;

	UAbilitySystemComponent* ASC = PC->GetAbilitySystemComponent();
	if (!ASC) return;

	const int32 PresetCount = GetTestSkillPresetCount();
	if (PresetCount <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server: TestSkillSet is not assigned or has no presets"));
		return;
	}

	ClearTestSkillSlots(ASC);
	EquipTestSkillPreset(PC, TestSkillPresetIndex);

	TestSkillPresetIndex = (TestSkillPresetIndex + 1) % PresetCount;
}


void AGP_PlayerController::ClearTestSkillSlots(UAbilitySystemComponent* ASC)
{
	if (!ASC) return;

	const TArray<FGameplayTag> SlotTags = { GPTags::Ability::Skill::Slot01, GPTags::Ability::Skill::Slot02 };
	TArray<FGameplayAbilitySpecHandle> HandlesToRemove;

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		for (const FGameplayTag& SlotTag : SlotTags)
		{
			if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
			{
				HandlesToRemove.Add(Spec.Handle);
			}
		}
	}

	for (const FGameplayAbilitySpecHandle& Handle : HandlesToRemove)
	{
		ASC->ClearAbility(Handle);
	}
}

int32 AGP_PlayerController::GetTestSkillPresetCount() const
{
	if (TestSkillSet && !TestSkillSet->Presets.IsEmpty())
	{
		return TestSkillSet->Presets.Num();
	}

	return 0;
}

void AGP_PlayerController::EquipTestSkillPreset(AGP_PlayerCharacter* PlayerCharacter, int32 PresetIndex)
{
	if (!PlayerCharacter) return;

	if (!TestSkillSet || !TestSkillSet->Presets.IsValidIndex(PresetIndex)) return;

	const FGP_TestSkillPreset& Preset = TestSkillSet->Presets[PresetIndex];

	if (Preset.Slot01Skill)
	{
		PlayerCharacter->EquipSkill(Preset.Slot01Skill, GPTags::Ability::Skill::Slot01, true);
	}

	if (Preset.Slot02Skill)
	{
		PlayerCharacter->EquipSkill(Preset.Slot02Skill, GPTags::Ability::Skill::Slot02, true);
	}

	const FString PresetName = Preset.PresetName.IsEmpty()
		? FString::Printf(TEXT("Preset %d"), PresetIndex)
		: Preset.PresetName.ToString();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan, FString::Printf(TEXT("%s equipped"), *PresetName));
	}

	UE_LOG(LogTemp, Warning, TEXT("Server: Test preset %d equipped from data asset (%s)"), PresetIndex, *PresetName);
}

void AGP_PlayerController::UpdateCharacterRotation(float DeltaSeconds)
{
	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}

	UCharacterMovementComponent* MoveComp = PlayerCharacter->GetCharacterMovement();
	if (!MoveComp)
	{
		return;
	}

	const bool bFixed = PlayerCharacter->GetAbilitySystemComponent() &&
		PlayerCharacter->GetAbilitySystemComponent()->HasMatchingGameplayTag(GPTags::State::Status::Fixed);

	if (bFixed)
	{
		MoveComp->bUseControllerDesiredRotation = false;
		return;
	}

	// 1. 이동 중일 때 (컨트롤러 원하는 각도 즉시 추종)
	if (!CurrentMoveInput.IsNearlyZero())
	{
		MoveComp->bUseControllerDesiredRotation = true;
		return;
	}

	// 2. 제자리(Idle)일 때
	MoveComp->bUseControllerDesiredRotation = false; // 제자리에서는 고정하여 루트 모션 회전 허용
}

bool AGP_PlayerController::ShouldSmoothMoveDirection() const
{
	AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PC) return false;

	UCharacterMovementComponent* MoveComp = PC->GetCharacterMovement();
	if (!MoveComp) return false;

	// 1. 공중 상태 예외
	if (MoveComp->IsFalling()) return false;

	// 2. LockOn 상태 예외
	if (PC->IsLockOn()) return false;

	// 3. GAS 상태 태그 체크 예외
	UAbilitySystemComponent* ASC = PC->GetAbilitySystemComponent();
	if (ASC)
	{
		FGameplayTag AimingTag = FGameplayTag::RequestGameplayTag(FName("State.Movement.Aiming"), false);
		FGameplayTag DashTag = FGameplayTag::RequestGameplayTag(FName("State.Movement.Dash"), false);
		FGameplayTag CombatTag = FGameplayTag::RequestGameplayTag(FName("State.Status.Combat"), false);
		FGameplayTag FixedTag = GPTags::State::Status::Fixed;

		if (AimingTag.IsValid() && ASC->HasMatchingGameplayTag(AimingTag)) return false;
		if (DashTag.IsValid() && ASC->HasMatchingGameplayTag(DashTag)) return false;
		if (CombatTag.IsValid() && ASC->HasMatchingGameplayTag(CombatTag)) return false;
		if (FixedTag.IsValid() && ASC->HasMatchingGameplayTag(FixedTag)) return false;
	}

	return true;
}

void AGP_PlayerController::ResetMoveDirectionSmoothing()
{
	SmoothedMoveWorldDirection = FVector::ZeroVector;
	bHasSmoothedMoveWorldDirection = false;
}
