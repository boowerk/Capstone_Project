#include "Player/GP_PlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/Character.h"

#include "GameFramework/PlayerState.h"
#include "Engine/World.h"

void AGP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;

	for (UInputMappingContext* Context : InputMappingContexts) {
		InputSubsystem->AddMappingContext(Context, 0);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ThisClass::StopJump);
	EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);

	EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &ThisClass::Primary);
	EnhancedInputComponent->BindAction(RollingAction, ETriggerEvent::Started, this, &ThisClass::Rolling);
}

void AGP_PlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn()))return;
	const FVector2D MovementVector = Value.Get<FVector2D>();

	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	GetPawn()->AddMovementInput(ForwardDirection, MovementVector.Y);
	GetPawn()->AddMovementInput(RightDirection, MovementVector.X);
}

void AGP_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn()))return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
}

void AGP_PlayerController::Jump()
{
	if(!IsValid(GetCharacter()))return;
	GetCharacter()->Jump();
}

void AGP_PlayerController::StopJump()
{
	if(!IsValid(GetCharacter()))return;
	GetCharacter()->StopJumping();
}

void AGP_PlayerController::Primary()
{
	UE_LOG(LogTemp, Warning, TEXT("Primary"));


}

void AGP_PlayerController::Rolling()
{
	UE_LOG(LogTemp, Warning, TEXT("Rolling"));
}