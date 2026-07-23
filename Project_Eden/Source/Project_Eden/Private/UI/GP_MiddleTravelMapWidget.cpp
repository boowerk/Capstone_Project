#include "UI/GP_MiddleTravelMapWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Player/GP_PlayerController.h"
#include "UI/GP_MinimapSubsystem.h"

UGP_MiddleTravelMapWidget::UGP_MiddleTravelMapWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MinimapMaterialFinder(
		TEXT("/Game/UI/HUD/Minimap/Materials/M_UI_Minimap_StaticMap.M_UI_Minimap_StaticMap"));
	MinimapMapMaterial = MinimapMaterialFinder.Object;
}

void UGP_MiddleTravelMapWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (DestinationButton0)
	{
		DestinationButton0->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDestination0Clicked);
	}
	if (DestinationButton1)
	{
		DestinationButton1->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDestination1Clicked);
	}
	if (DestinationButton2)
	{
		DestinationButton2->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDestination2Clicked);
	}
	if (DestinationButton3)
	{
		DestinationButton3->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleDestination3Clicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleCloseClicked);
	}

	RefreshTravelMap();
}

void UGP_MiddleTravelMapWidget::SetDestinations(
	const TArray<FGPMiddleTravelDestination>& InDestinations)
{
	Destinations = InDestinations;
	Destinations.Sort(
		[](const FGPMiddleTravelDestination& A, const FGPMiddleTravelDestination& B)
		{
			if (A.ZoneOrder != B.ZoneOrder)
			{
				return A.ZoneOrder < B.ZoneOrder;
			}
			return A.ZoneId.LexicalLess(B.ZoneId);
		});
	RefreshTravelMap();
}

void UGP_MiddleTravelMapWidget::CloseTravelMap()
{
	if (AGP_PlayerController* PlayerController =
		Cast<AGP_PlayerController>(GetOwningPlayer()))
	{
		PlayerController->CloseMiddleTravelMap();
		return;
	}

	RemoveFromParent();
}

void UGP_MiddleTravelMapWidget::RefreshTravelMap()
{
	UGP_MinimapSubsystem* MinimapSubsystem =
		GetWorld() ? GetWorld()->GetSubsystem<UGP_MinimapSubsystem>() : nullptr;
	if (MapImage && MinimapSubsystem)
	{
		UTextureRenderTarget2D* RenderTarget =
			MinimapSubsystem->GetMinimapRenderTarget();
		if (IsValid(RenderTarget))
		{
			if (!IsValid(MinimapMaterialInstance) && IsValid(MinimapMapMaterial))
			{
				MinimapMaterialInstance =
					UMaterialInstanceDynamic::Create(MinimapMapMaterial, this);
			}

			FSlateBrush MapBrush = MapImage->GetBrush();
			if (IsValid(MinimapMaterialInstance))
			{
				MinimapMaterialInstance->SetTextureParameterValue(
					TEXT("MapTexture"),
					RenderTarget);
				MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapCenterU"), 0.5f);
				MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapCenterV"), 0.5f);
				MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapZoom"), 1.0f);
				// The travel map is square; disable the HUD's circular crop.
				MinimapMaterialInstance->SetScalarParameterValue(
					TEXT("CircleMaskRadius"),
					1.0f);
				MapBrush.SetResourceObject(MinimapMaterialInstance);
			}
			else
			{
				MapBrush.SetResourceObject(RenderTarget);
			}
			MapImage->SetColorAndOpacity(FLinearColor::White);
			MapImage->SetBrush(MapBrush);
		}
	}

	const TArray<UButton*> Buttons = GetDestinationButtons();
	for (int32 ButtonIndex = 0; ButtonIndex < Buttons.Num(); ++ButtonIndex)
	{
		UButton* Button = Buttons[ButtonIndex];
		if (!Button)
		{
			continue;
		}

		const bool bHasDestination = Destinations.IsValidIndex(ButtonIndex);
		Button->SetVisibility(
			bHasDestination ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		Button->SetIsEnabled(bHasDestination);
		if (!bHasDestination || !MinimapSubsystem)
		{
			continue;
		}

		FVector2D MapUV;
		if (!MinimapSubsystem->WorldToMapUV(
			Destinations[ButtonIndex].WorldLocation,
			MapUV))
		{
			Button->SetVisibility(ESlateVisibility::Collapsed);
			continue;
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Button->Slot))
		{
			CanvasSlot->SetAutoSize(false);
			CanvasSlot->SetSize(FVector2D(DestinationMarkerSize));
			CanvasSlot->SetPosition(
				FVector2D(MapUV.X * MapDesignSize.X, MapUV.Y * MapDesignSize.Y)
				- FVector2D(DestinationMarkerSize * 0.5f));
		}

		if (UTextBlock* Label = Cast<UTextBlock>(Button->GetChildAt(0)))
		{
			Label->SetText(FText::FromName(Destinations[ButtonIndex].ZoneId));
		}
	}
}

void UGP_MiddleTravelMapWidget::SelectDestination(int32 DestinationIndex)
{
	if (!Destinations.IsValidIndex(DestinationIndex))
	{
		return;
	}

	if (AGP_PlayerController* PlayerController =
		Cast<AGP_PlayerController>(GetOwningPlayer()))
	{
		PlayerController->RequestMiddleTravel(Destinations[DestinationIndex].ZoneId);
	}
}

TArray<UButton*> UGP_MiddleTravelMapWidget::GetDestinationButtons() const
{
	return {
		DestinationButton0,
		DestinationButton1,
		DestinationButton2,
		DestinationButton3
	};
}

void UGP_MiddleTravelMapWidget::HandleDestination0Clicked()
{
	SelectDestination(0);
}

void UGP_MiddleTravelMapWidget::HandleDestination1Clicked()
{
	SelectDestination(1);
}

void UGP_MiddleTravelMapWidget::HandleDestination2Clicked()
{
	SelectDestination(2);
}

void UGP_MiddleTravelMapWidget::HandleDestination3Clicked()
{
	SelectDestination(3);
}

void UGP_MiddleTravelMapWidget::HandleCloseClicked()
{
	CloseTravelMap();
}
