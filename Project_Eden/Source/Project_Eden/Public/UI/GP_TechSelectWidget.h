#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "GP_TechSelectWidget.generated.h"

class AGP_PlayerState;
class UButton;

UCLASS()
class PROJECT_EDEN_API UGP_TechSelectWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Tech")
	bool SelectTechElement(FGameplayTag ElementTag);

	UFUNCTION(BlueprintPure, Category = "Tech")
	FGameplayTag GetCurrentTechElementTag() const;

protected:
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Tech")
	void OnTechElementSelected(FGameplayTag ElementTag);

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Pyros;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Hydro;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Volt;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Aero;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Lux;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Chaos;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> Button_Brute;

	UFUNCTION()
	void SelectPyros();

	UFUNCTION()
	void SelectHydro();

	UFUNCTION()
	void SelectVolt();

	UFUNCTION()
	void SelectAero();

	UFUNCTION()
	void SelectLux();

	UFUNCTION()
	void SelectChaos();

	UFUNCTION()
	void SelectBrute();

	AGP_PlayerState* GetGPPlayerState() const;
};
