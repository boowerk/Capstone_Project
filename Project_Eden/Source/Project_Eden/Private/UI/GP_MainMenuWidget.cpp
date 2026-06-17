#include "UI/GP_MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"

void UGP_MainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Host)
	{
		Button_Host->OnClicked.AddDynamic(this, &UGP_MainMenuWidget::OnHostClicked);
	}
	if (Button_Join)
	{
		Button_Join->OnClicked.AddDynamic(this, &UGP_MainMenuWidget::OnJoinClicked);
	}
	if (Button_Quit)
	{
		Button_Quit->OnClicked.AddDynamic(this, &UGP_MainMenuWidget::OnQuitClicked);
	}
}

void UGP_MainMenuWidget::OnHostClicked()
{
	const FString URL = FString::Printf(TEXT("/Game/Maps/%s?listen"), *LobbyMapName);
	UGameplayStatics::OpenLevel(this, FName(*URL));
}

void UGP_MainMenuWidget::OnJoinClicked()
{
	FString IP = TEXT("127.0.0.1:7778");
	if (TextBox_IP && !TextBox_IP->GetText().IsEmpty())
	{
		IP = TextBox_IP->GetText().ToString();
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->ClientTravel(IP, TRAVEL_Absolute);
	}
}

void UGP_MainMenuWidget::OnQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}
