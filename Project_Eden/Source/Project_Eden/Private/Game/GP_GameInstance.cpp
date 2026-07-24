#include "Game/GP_GameInstance.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

void UGP_GameInstance::Init()
{
	Super::Init();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().AddUObject(this, &UGP_GameInstance::HandleNetworkFailure);
		GEngine->OnTravelFailure().AddUObject(this, &UGP_GameInstance::HandleTravelFailure);
	}
}

void UGP_GameInstance::Shutdown()
{
	HideInitialOuterLoadingScreen();

	if (GEngine)
	{
		GEngine->OnNetworkFailure().RemoveAll(this);
		GEngine->OnTravelFailure().RemoveAll(this);
	}

	Super::Shutdown();
}

void UGP_GameInstance::ShowInitialOuterLoadingScreen()
{
	bInitialOuterLoadingActive = true;
	if (InitialOuterLoadingWidget.IsValid())
	{
		return;
	}

	UGameViewportClient* ViewportClient = GetGameViewportClient();
	if (!IsValid(ViewportClient))
	{
		return;
	}

	InitialOuterLoadingWidget =
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SBorder)
			.BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
			.BorderBackgroundColor(FLinearColor::Black)
			.Padding(0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT(
							"ProjectEden",
							"InitialOuterLoading",
							"LOADING..."))
						.Font(FCoreStyle::GetDefaultFontStyle(TEXT("Bold"), 30))
						.ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.0f, 20.0f, 0.0f, 0.0f)
					[
						SNew(SThrobber)
						.NumPieces(3)
					]
				]
			]
		];

	ViewportClient->AddViewportWidgetContent(
		InitialOuterLoadingWidget.ToSharedRef(),
		10000);
	UE_LOG(LogTemp, Log, TEXT("[Loading] Showing initial Outer loading screen."));
}

void UGP_GameInstance::HideInitialOuterLoadingScreen()
{
	bInitialOuterLoadingActive = false;

	if (InitialOuterLoadingWidget.IsValid())
	{
		if (UGameViewportClient* ViewportClient = GetGameViewportClient())
		{
			ViewportClient->RemoveViewportWidgetContent(
				InitialOuterLoadingWidget.ToSharedRef());
		}
		InitialOuterLoadingWidget.Reset();
		UE_LOG(LogTemp, Log, TEXT("[Loading] Hidden after initial Outer placement."));
	}
}

bool UGP_GameInstance::IsAuthority(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}
	const ENetMode NetMode = World->GetNetMode();
	return NetMode == NM_DedicatedServer || NetMode == NM_ListenServer;
}

void UGP_GameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* /*NetDriver*/,
	ENetworkFailure::Type /*FailureType*/, const FString& ErrorString)
{
	// A client dropping off the host fires here on the host too; don't yank the
	// host to the menu — Logout already handles a departed client there.
	if (IsAuthority(World))
	{
		return;
	}

	LastConnectionError = ErrorString.IsEmpty()
		? TEXT("Connection lost.")
		: ErrorString;

	ReturnToMainMenu();
}

void UGP_GameInstance::HandleTravelFailure(UWorld* World, ETravelFailure::Type /*FailureType*/,
	const FString& ErrorString)
{
	if (IsAuthority(World))
	{
		return;
	}

	LastConnectionError = ErrorString.IsEmpty()
		? TEXT("Travel failed.")
		: ErrorString;

	ReturnToMainMenu();
}

void UGP_GameInstance::ReturnToMainMenu()
{
	HideInitialOuterLoadingScreen();

	// Absolute travel disconnects from any server and loads the menu locally,
	// which also tears down a listen server when the host leaves.
	UGameplayStatics::OpenLevel(this, FName(*MainMenuMapName), /*bAbsolute=*/true);
}
