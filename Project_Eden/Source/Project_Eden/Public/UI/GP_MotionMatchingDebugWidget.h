#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GP_MotionMatchingDebugWidget.generated.h"

class UTextBlock;
class UGP_MotionMatchingAnimInstance;

/**
 * Motion Matching 상태를 실시간으로 모니터링하기 위한 디버그 위젯
 */
UCLASS()
class PROJECT_EDEN_API UGP_MotionMatchingDebugWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 현재 캐릭터의 속도 (GroundSpeed) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Speed;

	/** 현재 Gait 상태 (Walk, Run, Sprint) */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_Gait;

	/** Start 판정 활성화 여부 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_IsStarting;

	/** Pivot 판정 활성화 여부 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> Text_IsPivoting;

	/** 애니메이션 인스턴스 참조를 가져오는 헬퍼 함수 */
	UGP_MotionMatchingAnimInstance* GetAnimInstance() const;
};
