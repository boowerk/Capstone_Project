#include "UI/GP_LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/GP_LobbyPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"

void UGP_LobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Ready)
	{
		Button_Ready->OnClicked.AddDynamic(this, &UGP_LobbyWidget::OnReadyClicked);
	}

	BindToPlayerStates();
	RefreshPlayerList();
}

void UGP_LobbyWidget::NativeDestruct()
{
	UnbindFromPlayerStates();
	Super::NativeDestruct();
}

void UGP_LobbyWidget::OnReadyClicked()
{
	bIsReady = !bIsReady;

	if (AGP_LobbyPlayerState* LPS = GetOwningPlayerState<AGP_LobbyPlayerState>())
	{
		LPS->ServerSetReady(bIsReady);
	}

	// Update button text to reflect current state.
	if (Button_Ready)
	{
		// BP can style this further via OnReadyChanged delegate.
	}
}

void UGP_LobbyWidget::RefreshPlayerList()
{
	if (!Box_PlayerList)
	{
		return;
	}

	Box_PlayerList->ClearChildren();

	const AGameStateBase* GS = GetWorld()->GetGameState();
	if (!GS)
	{
		return;
	}

	const int32 Total = GS->PlayerArray.Num();

	if (Text_PlayerCount)
	{
		Text_PlayerCount->SetText(FText::FromString(FString::Printf(TEXT("%d / 3"), Total)));
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		AGP_LobbyPlayerState* LPS = Cast<AGP_LobbyPlayerState>(PS);
		if (!LPS)
		{
			continue;
		}

		const FString Name = LPS->GetPlayerName();
		const FString Status = LPS->IsReady() ? TEXT("준비완료") : TEXT("대기중...");

		if (PlayerRowWidgetClass)
		{
			UUserWidget* Row = CreateWidget<UUserWidget>(GetOwningPlayer(), PlayerRowWidgetClass);
			if (Row)
			{
				// BP row widget: expose Text_Name and Text_Status as BindWidget variables,
				// then cast and set them here, or just use SetContentForSlot in BP.
				Box_PlayerList->AddChildToVerticalBox(Row);
			}
		}
		else
		{
			// Fallback: plain text row when no row widget class is set.
			UTextBlock* Row = NewObject<UTextBlock>(this);
			Row->SetText(FText::FromString(FString::Printf(TEXT("%s   %s"), *Name, *Status)));
			Box_PlayerList->AddChildToVerticalBox(Row);
		}
	}
}

void UGP_LobbyWidget::BindToPlayerStates()
{
	const AGameStateBase* GS = GetWorld() ? GetWorld()->GetGameState() : nullptr;
	if (!GS)
	{
		return;
	}

	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AGP_LobbyPlayerState* LPS = Cast<AGP_LobbyPlayerState>(PS))
		{
			LPS->OnReadyChanged.AddDynamic(this, &UGP_LobbyWidget::OnPlayerReadyChanged);
			BoundPlayerStates.Add(LPS);
		}
	}
}

void UGP_LobbyWidget::UnbindFromPlayerStates()
{
	for (AGP_LobbyPlayerState* LPS : BoundPlayerStates)
	{
		if (IsValid(LPS))
		{
			LPS->OnReadyChanged.RemoveDynamic(this, &UGP_LobbyWidget::OnPlayerReadyChanged);
		}
	}
	BoundPlayerStates.Empty();
}

void UGP_LobbyWidget::OnPlayerReadyChanged(AGP_LobbyPlayerState* PlayerState, bool bReady)
{
	RefreshPlayerList();
}
