#include "UI/GP_MotionMatchingDebugWidget.h"
#include "Components/TextBlock.h"
#include "Animation/GP_MotionMatchingAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UGP_MotionMatchingDebugWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 틱이 도는지 확인 (화면 왼쪽 상단에 매 프레임 찍혀야 함)
	// GEngine->AddOnScreenDebugMessage(1234, 0.1f, FColor::Cyan, TEXT("Widget Ticking..."));

	UGP_MotionMatchingAnimInstance* AnimInst = GetAnimInstance();
	if (!AnimInst)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(5678, 0.1f, FColor::Red, TEXT("Error: AnimInst is NULL! Check AnimBP Parent Class."));
		return;
	}

	// 1. 속도 업데이트
	if (Text_Speed)
	{
		Text_Speed->SetText(FText::AsNumber(FMath::RoundToInt(AnimInst->GetGroundSpeed())));
	}

	// 2. Gait 업데이트
	if (Text_Gait)
	{
		FString GaitString;
		switch (AnimInst->GetGait())
		{
		case E_Gait::Walk:   GaitString = TEXT("Walk"); break;
		case E_Gait::Run:    GaitString = TEXT("Run"); break;
		case E_Gait::Sprint: GaitString = TEXT("Sprint"); break;
		default:             GaitString = TEXT("None"); break;
		}
		Text_Gait->SetText(FText::FromString(GaitString));
	}

	// 3. Start/Pivot 상태 업데이트 (ASCII 기반 시각화)
	auto SetStatusText = [](UTextBlock* TextBlock, bool bActive)
	{
		if (TextBlock)
		{
			TextBlock->SetText(FText::FromString(bActive ? TEXT("[ACTIVE]") : TEXT("---")));
			TextBlock->SetColorAndOpacity(FSlateColor(bActive ? FLinearColor::Green : FLinearColor::Gray));
		}
	};

	SetStatusText(Text_IsStarting, AnimInst->GetIsStarting());
	SetStatusText(Text_IsPivoting, AnimInst->GetIsPivoting());
}

UGP_MotionMatchingAnimInstance* UGP_MotionMatchingDebugWidget::GetAnimInstance() const
{
	ACharacter* PlayerCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	if (!PlayerCharacter) return nullptr;

	UAnimInstance* AnimInst = PlayerCharacter->GetMesh()->GetAnimInstance();
	UGP_MotionMatchingAnimInstance* MMAnimInst = Cast<UGP_MotionMatchingAnimInstance>(AnimInst);

	// 만약 위젯은 뜨는데 글자가 안 바뀐다면, 이 로그가 찍힐 것입니다.
	if (!MMAnimInst && GEngine)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::Red, TEXT("Debug Error: AnimInstance is NOT GP_MotionMatchingAnimInstance!"));
	}

	return MMAnimInst;
}
