#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_AnimationTypes.h"
#include "GP_MotionMatchingAnimInstance.generated.h"

class UPoseSearchDatabase;

/**
 * Motion Matching 및 Chooser를 사용하는 애니메이션 블루프린트를 위한 전용 애님 인스턴스.
 * ABP_UEFN_Puppet 등에서 부모 클래스로 사용됩니다.
 */
UCLASS()
class PROJECT_EDEN_API UGP_MotionMatchingAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	UGP_MotionMatchingAnimInstance();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Chooser 또는 로직에 의해 선택된 현재 포즈 검색 데이터베이스 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	/** Chooser 입력 및 MM 상태 전용 변수들 (승격된 C++ Enum 사용) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	E_Gait Gait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	E_Stance Stance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	E_MovementMode MovementMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	E_MovementState MovementState;

	/** Chooser를 통해 적절한 PoseSearchDatabase를 선택하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	virtual void UpdateMotionMatchingState();
};
