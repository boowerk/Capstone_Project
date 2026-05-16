#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "PDA_CharacterAnimationSet.h"
#include "GP_CharacterAnimInstance.generated.h"

class ACharacter;
class UCharacterMovementComponent;
class UBlendSpace;
class UAnimSequenceBase;

/**
 * ������Ʈ�� ��� ĳ���� �ִϸ��̼� ��������Ʈ�� ���� ���� ���̽� Ŭ����
 */
UCLASS()
class PROJECT_EDEN_API UGP_CharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UGP_CharacterAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** �ܺ�(ĳ���� ��)���� �ִϸ��̼� ��Ʈ�� �������� �����ϱ� ���� �Լ� */
	UFUNCTION(BlueprintCallable, Category = "AnimationData")
	void SetAnimationSet(UPDA_CharacterAnimationSet* NewSet);

/** 애니메이션 셋의 에셋들을 실시간으로 강제 적용할지 여부 */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AnimationData")
bool bForceApplyAnimationSet = true;

protected:
/** ĳͺ ִϸ̼ Ʈ  */
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AnimationData")
TObjectPtr<UPDA_CharacterAnimationSet> AnimationSet;


	// === Runtime Cached Assets (AnimGraph���� ���� ���) ===
	
	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UBlendSpace> LocomotionBlendSpace;

	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UAnimSequenceBase> JumpLoopAnimation;

	UPROPERTY(BlueprintReadOnly, Category = "AnimationData|Cached")
	TObjectPtr<UAnimSequenceBase> SprintStopAnimation;

	/** ���� ���� ���� ĳ���� ĳ�� */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<ACharacter> Character;

	/** ĳ������ �����Ʈ ������Ʈ ĳ�� */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;

	// === Locomotion Data (ĳ���� ����) ===
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector Velocity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector Acceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	float GroundSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	FVector LocalVelocityDirection;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float LocalVelocityAngleDegrees;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float LocalAccelerationAngleDegrees;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    FVector2D MoveInput;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    bool bHasMoveInput;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float ControlYawDelta;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float ControlYawDeltaRate;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
    float TurnRate;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsFalling;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bHasAcceleration;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsCrouching;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Locomotion")
	bool bIsAnyMontagePlaying;

private:
    float PreviousActorYaw = 0.0f;
    float PreviousControlYawDelta = 0.0f;
};
