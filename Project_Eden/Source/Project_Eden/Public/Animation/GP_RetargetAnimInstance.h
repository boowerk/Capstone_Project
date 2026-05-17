#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GP_AnimationTypes.h"
#include "GP_RetargetAnimInstance.generated.h"

/**
 * [리타겟 전용] 퍼펫(Source)으로부터 포즈와 상태만 전달받는 경량 애니메이션 인스턴스.
 * 불필요한 물리 계산을 수행하지 않고, AnimGraph 후처리 및 분기에 필요한 변수만 보유합니다.
 */
UCLASS()
class PROJECT_EDEN_API UGP_RetargetAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UGP_RetargetAnimInstance();

protected:
	/** 퍼펫으로부터 동기화받을 로코모션 상태 변수들 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retarget|State")
	E_Gait Gait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retarget|State")
	E_Stance Stance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retarget|State")
	E_ExperimentalStateMachineState ExperimentalState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retarget|State")
	E_MovementDirection MovementDirection;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Retarget|State")
	float GroundSpeed;

public:
	// 상태를 외부(퍼펫 등)에서 주입하기 위한 Setter
	UFUNCTION(BlueprintCallable, Category = "Retarget|Sync")
	void SyncLocomotionState(E_Gait InGait, E_Stance InStance, E_ExperimentalStateMachineState InExpState, E_MovementDirection InDirection, float InSpeed);
};
