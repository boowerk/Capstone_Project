#include "Player/GP_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Abilities/GP_SkillAugmentData.h"
#include "AbilitySystem/Abilities/GP_SkillAugmentPoolData.h"
#include "AbilitySystem/Abilities/GP_TestSkillSet.h"
#include "Blueprint/UserWidget.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Characters/GP_PlayerCharacter.h"
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayTags/GP_Tags.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Logging/LogMacros.h"
#include "Player/GP_PlayerState.h"
#include "UI/GP_AugmentSelectWidget.h"
#include "UI/GP_CharacterStatsMenuWidget.h"
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

bool AGP_PlayerController::RequestOpenAugmentSelect()
{
	if (!IsLocalController())
	{
		return false;
	}

	if (!IsValid(AugmentPoolData))
	{
		UE_LOG(LogTemp, Warning, TEXT("AugmentPoolData is not set on %s."), *GetName());
		return false;
	}

	AGP_PlayerState* GPPlayerState = GetPlayerState<AGP_PlayerState>();
	TArray<UGP_SkillAugmentData*> ExcludedAugments;
	FGameplayTag CurrentElementTag;
	if (IsValid(GPPlayerState))
	{
		ExcludedAugments = GPPlayerState->GetSelectedSkillAugments();
		CurrentElementTag = GPPlayerState->GetCurrentTechElementTag();
	}

	TArray<UGP_SkillAugmentData*> CandidateAugments = AugmentPoolData->PickRandomAugmentsExcludingForElement(
		AugmentCandidateCount,
		ExcludedAugments,
		CurrentElementTag);
	return OpenAugmentSelectWidget(CandidateAugments);
}

bool AGP_PlayerController::OpenAugmentSelectWidget(const TArray<UGP_SkillAugmentData*>& Candidates)
{
	if (!IsLocalController() || Candidates.IsEmpty())
	{
		return false;
	}

	if (!IsValid(AugmentSelectWidget))
	{
		if (!AugmentSelectWidgetClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("AugmentSelectWidgetClass is not set on %s."), *GetName());
			return false;
		}

		AugmentSelectWidget = CreateWidget<UGP_AugmentSelectWidget>(this, AugmentSelectWidgetClass);
		if (!IsValid(AugmentSelectWidget))
		{
			return false;
		}
	}

	AugmentSelectWidget->SetCandidateAugments(Candidates);
	if (!AugmentSelectWidget->IsInViewport())
	{
		AugmentSelectWidget->AddToViewport(80);
	}

	bShowMouseCursor = true;
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	FInputModeGameAndUI InputMode;
	InputMode.SetWidgetToFocus(AugmentSelectWidget->TakeWidget());
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);

	return true;
}

void AGP_PlayerController::CloseAugmentSelectWidget()
{
	if (IsValid(AugmentSelectWidget))
	{
		AugmentSelectWidget->RemoveFromParent();
		AugmentSelectWidget = nullptr;
	}

	bShowMouseCursor = false;
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	SetInputMode(FInputModeGameOnly());
}

void AGP_PlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMovementSpeed(DeltaSeconds);
	UpdateCharacterRotation(DeltaSeconds);
	UpdateSkillSelectionInputMode();

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
	if (CrouchAction)
	{
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::Input_CrouchPressed);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ThisClass::Input_CrouchReleased);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Canceled, this, &ThisClass::Input_CrouchReleased);
	}
	InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ThisClass::Input_CrouchPressed);
	InputComponent->BindKey(EKeys::C, IE_Released, this, &ThisClass::Input_CrouchReleased);

	// --- [상태 전환 ] ---
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ThisClass::Input_SprintPressed);
	EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &ThisClass::Input_SprintReleased);
	if (DashAction)   EnhancedInputComponent->BindAction(DashAction,   ETriggerEvent::Started, this, &ThisClass::Input_Dash);

	// --- [어빌리티 및 스킬] ---
	if (PrimaryAttackAction)  EnhancedInputComponent->BindAction(PrimaryAttackAction,  ETriggerEvent::Started,   this, &ThisClass::Input_PrimaryAttack);
	if (SecondarySkillConfirmAction) EnhancedInputComponent->BindAction(SecondarySkillConfirmAction, ETriggerEvent::Started, this, &ThisClass::Input_SecondarySkillConfirm);
	if (CancelSkillSelectionAction) EnhancedInputComponent->BindAction(CancelSkillSelectionAction, ETriggerEvent::Started, this, &ThisClass::Input_CancelSkillSelection);
	if (SkillSlot1Action)
	{
		EnhancedInputComponent->BindAction(SkillSlot1Action, ETriggerEvent::Started, this, &ThisClass::Input_SkillSlot1);
		EnhancedInputComponent->BindAction(SkillSlot1Action, ETriggerEvent::Completed, this, &ThisClass::Input_SkillSlotReleased);
		EnhancedInputComponent->BindAction(SkillSlot1Action, ETriggerEvent::Canceled, this, &ThisClass::Input_SkillSlotReleased);
	}
	if (SkillSlot2Action)
	{
		EnhancedInputComponent->BindAction(SkillSlot2Action, ETriggerEvent::Started, this, &ThisClass::Input_SkillSlot2);
		EnhancedInputComponent->BindAction(SkillSlot2Action, ETriggerEvent::Completed, this, &ThisClass::Input_SkillSlotReleased);
		EnhancedInputComponent->BindAction(SkillSlot2Action, ETriggerEvent::Canceled, this, &ThisClass::Input_SkillSlotReleased);
	}
	if (UltimateAction)
	{
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Started, this, &ThisClass::Input_UltimateSkill);
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Completed, this, &ThisClass::Input_SkillSlotReleased);
		EnhancedInputComponent->BindAction(UltimateAction, ETriggerEvent::Canceled, this, &ThisClass::Input_SkillSlotReleased);
	}
	if (TestToggleSkillAction) EnhancedInputComponent->BindAction(TestToggleSkillAction, ETriggerEvent::Started, this, &ThisClass::Input_TestToggleSkill);
	// Keep both debug bindings after the PR merge so neither test preset rotation nor White Void input is dropped.
	if (RotateTestSkillAction) EnhancedInputComponent->BindAction(RotateTestSkillAction, ETriggerEvent::Started, this, &ThisClass::Input_RotateTestSkill);
	if (WhiteVoidToggleAction) EnhancedInputComponent->BindAction(WhiteVoidToggleAction, ETriggerEvent::Started, this, &ThisClass::Input_ToggleWhiteVoid);

	// CharacterStatsMenuAction을 지정하지 않아도 Tab으로 능력치 메뉴 뼈대를 바로 테스트할 수 있게 fallback을 둡니다.
	if (CharacterStatsMenuAction)
	{
		EnhancedInputComponent->BindAction(CharacterStatsMenuAction, ETriggerEvent::Started, this, &ThisClass::Input_ToggleCharacterStatsMenu);
	}
	else
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ThisClass::Input_ToggleCharacterStatsMenu);
	}
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
	const bool bSprinting = PlayerCharacter && PlayerCharacter->IsSprinting() && !PlayerCharacter->IsPrimaryAttackInProgress();
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
		PlayerCharacter->ClearActionRootMotionCancelMovementInput();
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

void AGP_PlayerController::Input_CrouchPressed()
{
	CancelSkillSelectionIfActive();
	bCrouchInputHeld = true;

	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CrouchInput] Pressed Actor=%s CanCrouch=%d IsCrouched=%d"),
			*PC->GetName(),
			PC->GetCharacterMovement() ? (PC->GetCharacterMovement()->NavAgentProps.bCanCrouch ? 1 : 0) : 0,
			PC->bIsCrouched ? 1 : 0);
		PC->StopSprinting();
		PC->Crouch();
	}
}

void AGP_PlayerController::Input_CrouchReleased()
{
	bCrouchInputHeld = false;
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CrouchInput] Released Actor=%s IsCrouched=%d"),
			*PC->GetName(),
			PC->bIsCrouched ? 1 : 0);
		if (PC->IsPrimaryAttackInProgress())
		{
			return;
		}

		PC->UnCrouch();

		if (ShouldResumeHeldSprint())
		{
			PC->StartSprinting();
		}
	}
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
	CancelSkillSelectionIfActive();
	bSprintInputHeld = true;

	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		if (bIsSprintToggle) PC->ToggleSprinting();
		else PC->StartSprinting();
	}
}

void AGP_PlayerController::Input_SprintReleased()
{
	bSprintInputHeld = false;
	if (AGP_PlayerCharacter* PC = Cast<AGP_PlayerCharacter>(GetPawn()))
	{
		if (!bIsSprintToggle) PC->StopSprinting();
	}
}
void AGP_PlayerController::Input_Dash()
{
	CancelSkillSelectionIfActive();

	if (AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetCharacter()))
	{
		if (PlayerCharacter->TryPerformDash())
		{
			return;
		}
	}

	if (ActivateAbilityByTag(GPTags::Ability::Movement::Dash))
	{
		return;
	}
}


void AGP_PlayerController::Input_PrimaryAttack()
{
	if (IsSkillSelectionActive())
	{
		SendSkillSelectionEvent(GPTags::Event::Skill::ConfirmPrimary);
		return;
	}

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

void AGP_PlayerController::Input_SecondarySkillConfirm()
{
	if (IsSkillSelectionActive())
	{
		SendSkillSelectionEvent(GPTags::Event::Skill::ConfirmSecondary);
	}
}

void AGP_PlayerController::Input_CancelSkillSelection()
{
	SendSkillSelectionEvent(GPTags::Event::Skill::Cancel);
}

void AGP_PlayerController::Input_SkillSlot1()
{
	CancelSkillSelectionIfActive();
	UE_LOG(LogTemp, Warning, TEXT("Targeting"));
	ActivateAbilityByTag(GPTags::Ability::Skill::Slot01);
}

void AGP_PlayerController::Input_SkillSlot2()
{
	CancelSkillSelectionIfActive();
	ActivateAbilityByTag(GPTags::Ability::Skill::Slot02);
}

void AGP_PlayerController::Input_UltimateSkill()
{
	CancelSkillSelectionIfActive();
	ActivateAbilityByTag(GPTags::Ability::Skill::Ultimate);
}

void AGP_PlayerController::Input_SkillSlotReleased()
{
	if (IsSkillSelectionActive())
	{
		SendSkillSelectionEvent(GPTags::Event::Skill::ConfirmPrimary);
	}
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

bool AGP_PlayerController::IsSkillSelectionActive() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	return IsValid(ASC) && ASC->HasMatchingGameplayTag(GPTags::State::Skill::Selecting);
}

bool AGP_PlayerController::IsGroundPositionSelectionActive() const
{
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	return IsValid(ASC) && ASC->HasMatchingGameplayTag(GPTags::State::Skill::GroundPosition);
}

bool AGP_PlayerController::SendSkillSelectionEvent(const FGameplayTag& EventTag) const
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn) || !EventTag.IsValid())
	{
		return false;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = ControlledPawn;
	Payload.Target = ControlledPawn;

	FVector TargetLocation = FVector::ZeroVector;
	const bool bHasTargetLocation =
		EventTag.MatchesTagExact(GPTags::Event::Skill::ConfirmPrimary)
		&& IsGroundPositionSelectionActive()
		&& GetSkillSelectionCursorLocation(TargetLocation);
	if (bHasTargetLocation)
	{
		FillSkillSelectionTargetData(Payload, TargetLocation);
	}

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ControlledPawn, EventTag, Payload);

	if (!HasAuthority())
	{
		const_cast<AGP_PlayerController*>(this)->Server_SendSkillSelectionEvent(EventTag, TargetLocation, bHasTargetLocation);
	}

	return true;
}

void AGP_PlayerController::FillSkillSelectionTargetData(
	FGameplayEventData& Payload,
	const FVector& TargetLocation) const
{
	FGameplayAbilityTargetData_LocationInfo* LocationData = new FGameplayAbilityTargetData_LocationInfo();
	LocationData->TargetLocation.LocationType = EGameplayAbilityTargetingLocationType::LiteralTransform;
	LocationData->TargetLocation.LiteralTransform = FTransform(TargetLocation);
	Payload.TargetData.Add(LocationData);
}

bool AGP_PlayerController::GetSkillSelectionCursorLocation(FVector& OutTargetLocation) const
{
	FHitResult CursorHit;
	if (!GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
	{
		return false;
	}

	OutTargetLocation = CursorHit.ImpactPoint;
	return true;
}

void AGP_PlayerController::UpdateSkillSelectionInputMode()
{
	if (!IsLocalController())
	{
		return;
	}

	const bool bGroundSelectionActive = IsGroundPositionSelectionActive();
	if (bGroundSelectionActive == bWasGroundPositionSelectionActive)
	{
		return;
	}

	bWasGroundPositionSelectionActive = bGroundSelectionActive;
	if (bGroundSelectionActive)
	{
		bShowMouseCursor = true;
		SetIgnoreLookInput(true);

		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		return;
	}

	const bool bAugmentOpen = IsValid(AugmentSelectWidget) && AugmentSelectWidget->IsInViewport();
	if (!bAugmentOpen && !bIsCharacterStatsMenuOpen)
	{
		bShowMouseCursor = false;
		SetIgnoreLookInput(false);
		SetInputMode(FInputModeGameOnly());
	}
}

void AGP_PlayerController::CancelSkillSelectionIfActive() const
{
	if (IsSkillSelectionActive())
	{
		SendSkillSelectionEvent(GPTags::Event::Skill::Cancel);
	}
}

bool AGP_PlayerController::Server_SendSkillSelectionEvent_Validate(
	FGameplayTag EventTag,
	FVector_NetQuantize TargetLocation,
	bool bHasTargetLocation)
{
	const bool bValidEvent = EventTag.MatchesTagExact(GPTags::Event::Skill::ConfirmPrimary)
		|| EventTag.MatchesTagExact(GPTags::Event::Skill::ConfirmSecondary)
		|| EventTag.MatchesTagExact(GPTags::Event::Skill::Cancel);
	const bool bValidLocation = !bHasTargetLocation
		|| (FMath::IsFinite(TargetLocation.X)
			&& FMath::IsFinite(TargetLocation.Y)
			&& FMath::IsFinite(TargetLocation.Z));
	return bValidEvent && bValidLocation;
}

void AGP_PlayerController::Server_SendSkillSelectionEvent_Implementation(
	FGameplayTag EventTag,
	FVector_NetQuantize TargetLocation,
	bool bHasTargetLocation)
{
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn) || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = ControlledPawn;
	Payload.Target = ControlledPawn;
	if (bHasTargetLocation)
	{
		FillSkillSelectionTargetData(Payload, TargetLocation);
	}
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(ControlledPawn, EventTag, Payload);
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

bool AGP_PlayerController::EnsureCharacterStatsMenuWidget()
{
	if (IsValid(CharacterStatsMenuWidget))
	{
		return true;
	}

	if (!CharacterStatsMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("CharacterStatsMenuWidgetClass is not set on %s. Assign a Widget Blueprint based on GP_CharacterStatsMenuWidget."), *GetName());
		return false;
	}

	// 메뉴 WBP는 한 번 생성해두고 열고 닫을 때 Visibility만 바꿔 GAS 바인딩을 유지합니다.
	CharacterStatsMenuWidget = CreateWidget<UGP_CharacterStatsMenuWidget>(this, CharacterStatsMenuWidgetClass);
	if (!IsValid(CharacterStatsMenuWidget))
	{
		return false;
	}

	CharacterStatsMenuWidget->AddToViewport(50);
	CharacterStatsMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	return true;
}

void AGP_PlayerController::OpenCharacterStatsMenu()
{
	if (!EnsureCharacterStatsMenuWidget())
	{
		return;
	}

	CharacterStatsMenuWidget->InitializeForCharacter(Cast<AGP_BaseCharacter>(GetPawn()));
	CharacterStatsMenuWidget->SetVisibility(ESlateVisibility::Visible);
	bIsCharacterStatsMenuOpen = true;
	ApplyCharacterStatsMenuInputMode(true);
}

void AGP_PlayerController::CloseCharacterStatsMenu()
{
	if (IsValid(CharacterStatsMenuWidget))
	{
		CharacterStatsMenuWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	bIsCharacterStatsMenuOpen = false;
	ApplyCharacterStatsMenuInputMode(false);
}

void AGP_PlayerController::ApplyCharacterStatsMenuInputMode(bool bMenuOpen)
{
	bShowMouseCursor = bMenuOpen;
	SetIgnoreMoveInput(bMenuOpen);
	SetIgnoreLookInput(bMenuOpen);

	if (bMenuOpen)
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
	}
}

void AGP_PlayerController::UpdateMovementSpeed(float DeltaSeconds)
{
	AGP_PlayerCharacter* PlayerCharacter = Cast<AGP_PlayerCharacter>(GetPawn());
	if (!PlayerCharacter)
	{
		bHasTargetMaxWalkSpeed = false;
		return;
	}

	const bool bSprinting = PlayerCharacter->IsSprinting() && !PlayerCharacter->IsPrimaryAttackInProgress();
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

void AGP_PlayerController::Input_ToggleCharacterStatsMenu()
{
	if (bIsCharacterStatsMenuOpen)
	{
		CloseCharacterStatsMenu();
	}
	else
	{
		OpenCharacterStatsMenu();
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
