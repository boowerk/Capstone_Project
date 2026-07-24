#include "UI/GP_LobbyWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Game/GP_LobbyGameMode.h"
#include "Game/GP_LobbyPlayerController.h"
#include "Game/GP_LobbyPlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UI/GP_LobbyPlayerRowWidget.h"
#include "Game/GP_GameInstance.h"
#include "TimerManager.h"

void UGP_LobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Ready)
	{
		Button_Ready->OnClicked.AddDynamic(this, &UGP_LobbyWidget::OnReadyClicked);
	}

	if (Button_Leave)
	{
		Button_Leave->OnClicked.AddDynamic(this, &UGP_LobbyWidget::OnLeaveClicked);
	}

	if (Button_ForceStart)
	{
		AGP_LobbyPlayerController* LobbyController =
			GetOwningPlayer<AGP_LobbyPlayerController>();
		AGP_LobbyGameMode* LobbyGameMode = GetWorld()
			? GetWorld()->GetAuthGameMode<AGP_LobbyGameMode>()
			: nullptr;
		bool bCanForceStart = false;
#if !UE_BUILD_SHIPPING
		// Remote dedicated-server clients have no AuthGameMode. The matching
		// launch flag advertises the button; the server remains authoritative
		// and validates the same opt-in before accepting the RPC.
		bCanForceStart =
			FParse::Param(FCommandLine::Get(), TEXT("AllowLobbyForceStart"));
#endif
		if (LobbyGameMode)
		{
			bCanForceStart = LobbyGameMode->CanForceStart(LobbyController);
		}

		// Production and non-opted-in launches must not advertise the bypass.
		Button_ForceStart->SetVisibility(
			bCanForceStart ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bCanForceStart)
		{
			Button_ForceStart->OnClicked.AddDynamic(
				this,
				&UGP_LobbyWidget::OnForceStartClicked);
		}
	}

	BindToPlayerStates();
	RefreshPlayerList();

	// PlayerArray replicates without a change event, and players may join after
	// this widget is built, so poll periodically to keep the list in sync.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			if (IsValid(this) && !bIsLoading)
			{
				NotifyPlayerListChanged();
			}
		}), 0.5f, true);
	}
}

void UGP_LobbyWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimerHandle);
	}

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

void UGP_LobbyWidget::OnLeaveClicked()
{
	if (UGP_GameInstance* GameInstance = GetGameInstance<UGP_GameInstance>())
	{
		GameInstance->ReturnToMainMenu();
	}
}

void UGP_LobbyWidget::OnForceStartClicked()
{
	if (bIsLoading)
	{
		return;
	}

	if (AGP_LobbyPlayerController* LPC = GetOwningPlayer<AGP_LobbyPlayerController>())
	{
		LPC->ServerForceStart();
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
		FString Status;
		bool bShowReady;
		if (bIsLoading)
		{
			Status = TEXT("로딩중...");
			bShowReady = true;
		}
		else
		{
			bShowReady = LPS->IsReady();
			Status = bShowReady ? TEXT("준비완료") : TEXT("대기중...");
		}

		if (PlayerRowWidgetClass)
		{
			UGP_LobbyPlayerRowWidget* Row = CreateWidget<UGP_LobbyPlayerRowWidget>(GetOwningPlayer(), PlayerRowWidgetClass);
			if (Row)
			{
				Row->SetPlayerInfo(Name, bShowReady, bIsLoading);
				Box_PlayerList->AddChildToVerticalBox(Row);
			}
		}
		else
		{
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
			if (BoundPlayerStates.Contains(LPS))
			{
				continue;
			}
			LPS->OnReadyChanged.AddUniqueDynamic(this, &UGP_LobbyWidget::OnPlayerReadyChanged);
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

void UGP_LobbyWidget::NotifyPlayerListChanged()
{
	BindToPlayerStates();
	RefreshPlayerList();
}

void UGP_LobbyWidget::ShowLoadingState()
{
	bIsLoading = true;

	if (Button_Ready)
	{
		Button_Ready->SetIsEnabled(false);
	}

	RefreshPlayerList();
}

void UGP_LobbyWidget::HideLoadingState()
{
	bIsLoading = false;

	if (Button_Ready)
	{
		Button_Ready->SetIsEnabled(true);
	}

	RefreshPlayerList();
}
