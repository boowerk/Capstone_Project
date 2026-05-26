#include "UI/GP_TechSelectWidget.h"

#include "Components/Button.h"
#include "GameplayTags/GP_Tags.h"
#include "Player/GP_PlayerState.h"

void UGP_TechSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Button_Pyros) { Button_Pyros->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectPyros); }
	if (Button_Hydro) { Button_Hydro->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectHydro); }
	if (Button_Volt) { Button_Volt->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectVolt); }
	if (Button_Aero) { Button_Aero->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectAero); }
	if (Button_Lux) { Button_Lux->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectLux); }
	if (Button_Chaos) { Button_Chaos->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectChaos); }
	if (Button_Brute) { Button_Brute->OnClicked.AddDynamic(this, &UGP_TechSelectWidget::SelectBrute); }
}

bool UGP_TechSelectWidget::SelectTechElement(FGameplayTag ElementTag)
{
	if (!ElementTag.IsValid())
	{
		return false;
	}

	AGP_PlayerState* PlayerState = GetGPPlayerState();
	if (!IsValid(PlayerState))
	{
		return false;
	}

	PlayerState->SetCurrentTechElementTag(ElementTag);
	OnTechElementSelected(ElementTag);
	return true;
}

FGameplayTag UGP_TechSelectWidget::GetCurrentTechElementTag() const
{
	const AGP_PlayerState* PlayerState = GetGPPlayerState();
	return IsValid(PlayerState) ? PlayerState->GetCurrentTechElementTag() : FGameplayTag();
}

void UGP_TechSelectWidget::SelectPyros()
{
	SelectTechElement(GPTags::Tech::Element::Pyros);
}

void UGP_TechSelectWidget::SelectHydro()
{
	SelectTechElement(GPTags::Tech::Element::Hydro);
}

void UGP_TechSelectWidget::SelectVolt()
{
	SelectTechElement(GPTags::Tech::Element::Volt);
}

void UGP_TechSelectWidget::SelectAero()
{
	SelectTechElement(GPTags::Tech::Element::Aero);
}

void UGP_TechSelectWidget::SelectLux()
{
	SelectTechElement(GPTags::Tech::Element::Lux);
}

void UGP_TechSelectWidget::SelectChaos()
{
	SelectTechElement(GPTags::Tech::Element::Chaos);
}

void UGP_TechSelectWidget::SelectBrute()
{
	SelectTechElement(GPTags::Tech::Element::Brute);
}

AGP_PlayerState* UGP_TechSelectWidget::GetGPPlayerState() const
{
	APlayerController* OwningPlayer = GetOwningPlayer();
	return IsValid(OwningPlayer) ? OwningPlayer->GetPlayerState<AGP_PlayerState>() : nullptr;
}
