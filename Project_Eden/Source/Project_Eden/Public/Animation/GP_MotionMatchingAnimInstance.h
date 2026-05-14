#pragma once

#include "CoreMinimal.h"
#include "Animation/GP_CharacterAnimInstance.h"
#include "GP_MotionMatchingAnimInstance.generated.h"

class UPoseSearchDatabase;

/**
 * Motion Matching, Chooser를 사용하는 애니메이션 블루프린트를 위한 전용 애님 인스턴스
 * 기존 블루프린트 Enum 에셋 사용 
 * 경로는 (/Game/Blueprints/Data/E_*) 
 * 관리는 uint8로 상태 관리
 */
UCLASS()
class PROJECT_EDEN_API UGP_MotionMatchingAnimInstance : public UGP_CharacterAnimInstance
{
	GENERATED_BODY()

public:
	UGP_MotionMatchingAnimInstance();

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	/** Chooser 또는 로직으로 선택된 현재 포즈 검색 DB  */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MotionMatching")
	TObjectPtr<UPoseSearchDatabase> SelectedPoseSearchDatabase;

	/** 
	 * Chooser 입력 상태 변수들
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	uint8 Gait;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	uint8 Stance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	uint8 MovementMode;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MotionMatching|State")
	uint8 MovementState;

	/** Chooser를 통해 적절한 PoseSearchDatabase를 선택하는 함수 */
	UFUNCTION(BlueprintCallable, Category = "MotionMatching")
	virtual void UpdateMotionMatchingState();
};
