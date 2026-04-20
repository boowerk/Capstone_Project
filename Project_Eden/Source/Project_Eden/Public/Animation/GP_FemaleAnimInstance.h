#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

#include "GP_FemaleAnimInstance.generated.h"

class AGP_PlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class PROJECT_EDEN_API UGP_FemaleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<AGP_PlayerCharacter> Character;

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// === Locomotion Data ===
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bShouldSprintStop;
};
