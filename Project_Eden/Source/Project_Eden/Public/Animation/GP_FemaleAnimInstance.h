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
	UPROPERTY(BlueprintReadOnly, Category = "Movement") // 구식 함수 사용으로 메인스레드 점유율이 높아지는것을 방지하기위해 직접 추가
	bool bIsAnyMontagePlaying = false;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector LocalVelocityDirection = FVector::ZeroVector;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bHasAcceleration = false;

	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bShouldSprintStop = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling;
};
