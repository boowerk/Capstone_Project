#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Game/GP_MiddleTravelTypes.h"
#include "GP_MiddleTravelMapWidget.generated.h"

class AGP_PlayerController;
class UButton;
class UCanvasPanel;
class UImage;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * Full-map destination picker shown by an Outer village portal.
 *
 * The Widget Blueprint supplies the visual hierarchy. This native base reuses the existing
 * minimap render target and WorldToMapUV mapping, positions up to four clickable Middle markers,
 * and forwards the selected ZoneId to the owning player controller.
 */
UCLASS()
class PROJECT_EDEN_API UGP_MiddleTravelMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_MiddleTravelMapWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "Middle Travel")
	void SetDestinations(const TArray<FGPMiddleTravelDestination>& InDestinations);

	UFUNCTION(BlueprintCallable, Category = "Middle Travel")
	void CloseTravelMap();

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UImage> MapImage;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UCanvasPanel> DestinationCanvas;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DestinationButton0;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DestinationButton1;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DestinationButton2;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DestinationButton3;

	UPROPERTY(BlueprintReadOnly, Category = "Middle Travel", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	// Match this to the square DestinationCanvas size in the Widget Blueprint.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle Travel", meta = (ClampMin = "100.0"))
	FVector2D MapDesignSize = FVector2D(800.0f, 800.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Middle Travel", meta = (ClampMin = "8.0"))
	float DestinationMarkerSize = 48.0f;

private:
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> MinimapMapMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MinimapMaterialInstance;

	UPROPERTY(Transient)
	TArray<FGPMiddleTravelDestination> Destinations;

	void RefreshTravelMap();
	void SelectDestination(int32 DestinationIndex);
	TArray<UButton*> GetDestinationButtons() const;

	UFUNCTION()
	void HandleDestination0Clicked();

	UFUNCTION()
	void HandleDestination1Clicked();

	UFUNCTION()
	void HandleDestination2Clicked();

	UFUNCTION()
	void HandleDestination3Clicked();

	UFUNCTION()
	void HandleCloseClicked();
};
