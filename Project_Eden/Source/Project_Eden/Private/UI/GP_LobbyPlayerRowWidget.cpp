#include "UI/GP_LobbyPlayerRowWidget.h"

#include "Components/TextBlock.h"

void UGP_LobbyPlayerRowWidget::SetPlayerInfo(const FString& PlayerName, bool bReady, bool bIsLoading)
{
	if (Text_Name)
	{
		Text_Name->SetText(FText::FromString(PlayerName.IsEmpty() ? TEXT("Player") : PlayerName));
	}

	if (Text_Status)
	{
		FString StatusText;
		FLinearColor StatusColor;
		if (bIsLoading)
		{
			StatusText = TEXT("로딩중...");
			StatusColor = FLinearColor(1.0f, 0.85f, 0.0f);
		}
		else if (bReady)
		{
			StatusText = TEXT("준비완료");
			StatusColor = FLinearColor(0.18f, 0.80f, 0.44f);
		}
		else
		{
			StatusText = TEXT("대기중...");
			StatusColor = FLinearColor(0.67f, 0.67f, 0.67f);
		}
		Text_Status->SetText(FText::FromString(StatusText));
		Text_Status->SetColorAndOpacity(FSlateColor(StatusColor));
	}
}
