#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_AnimationTypes.h"
#include "GP_MotionMatchingAnimInstance.generated.h"

class UPoseSearchDatabase;

/**
 * [마이그레이션 단계 3] Motion Matching 전용 애님 인스턴스.
 * 승격된 C++ 에넘 타입을 직접 사용하여 타입 안전성을 확보합니다.
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

	/** 
	 * [단일 진실원] C++ 에넘 기반 상태 변수.
	 * 블루프린트 및 Chooser와의 타입 정렬을 보장합니다.
	 */
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
