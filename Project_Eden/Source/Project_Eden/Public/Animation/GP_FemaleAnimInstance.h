#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"

#include "GP_FemaleAnimInstance.generated.h"

class AGP_PlayerCharacter;
class UCharacterMovementComponent;

UCLASS()
class PROJECT_EDEN_API UGP_FemaleAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	// 추가적인 Female 전용 로직이 필요할 경우 여기에 작성
	UPROPERTY(BlueprintReadOnly, Category = "Character|Female")
	bool bShouldSprintStop = false;
};

