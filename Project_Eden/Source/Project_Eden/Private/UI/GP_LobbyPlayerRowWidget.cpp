#include "UI/GP_LobbyPlayerRowWidget.h"

#include "Components/TextBlock.h"

void UGP_LobbyPlayerRowWidget::SetPlayerInfo(const FString& PlayerName, bool bReady)
{
	if (Text_Name)
	{
		Text_Name->SetText(FText::FromString(PlayerName.IsEmpty() ? TEXT("Player") : PlayerName));
	}

	if (Text_Status)
	{
		Text_Status->SetText(FText::FromString(bReady ? TEXT("준비완료") : TEXT("대기중...")));
		Text_Status->SetColorAndOpacity(bReady
			? FSlateColor(FLinearColor(0.18f, 0.80f, 0.44f))
			: FSlateColor(FLinearColor(0.67f, 0.67f, 0.67f)));
	}
}
