#include "UI/GP_DebugAttributeWidget.h"
#include "Components/VerticalBox.h"
#include "Components/Border.h"
#include "UI/GP_DebugAttributeRow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/GP_AttributeSet.h"

void UGP_DebugAttributeWidget::InitializeDebugWidget()
{
	if (MainBackground)
	{
		MainBackground->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.7f));
	}

	if (!AttributeContainer || !RowWidgetClass) return;

	AttributeContainer->ClearChildren();

	AActor* OwningPawn = GetOwningPlayerPawn();
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwningPawn);
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;

	// 스탯 리스트 정의 (이름, 속성 추출 함수 활용)
	AddAttributeRow(TEXT("Health"), UGP_AttributeSet::GetHealthAttribute(), ASC);
	AddAttributeRow(TEXT("Max Health"), UGP_AttributeSet::GetMaxHealthAttribute(), ASC);
	AddAttributeRow(TEXT("Mana"), UGP_AttributeSet::GetManaAttribute(), ASC);
	AddAttributeRow(TEXT("Max Mana"), UGP_AttributeSet::GetMaxManaAttribute(), ASC);
	
	AddAttributeRow(TEXT("--- Physical ---"), FGameplayAttribute(), nullptr); // 구분선용
	AddAttributeRow(TEXT("Attack Power"), UGP_AttributeSet::GetAttackPowerAttribute(), ASC);
	AddAttributeRow(TEXT("Attack Speed"), UGP_AttributeSet::GetAttackSpeedAttribute(), ASC);
	AddAttributeRow(TEXT("Armor"), UGP_AttributeSet::GetArmorAttribute(), ASC);
	
	AddAttributeRow(TEXT("--- Offensive ---"), FGameplayAttribute(), nullptr);
	AddAttributeRow(TEXT("Magic Power"), UGP_AttributeSet::GetMagicPowerAttribute(), ASC);
	AddAttributeRow(TEXT("Crit Chance"), UGP_AttributeSet::GetCriticalChanceAttribute(), ASC);
	AddAttributeRow(TEXT("Crit Multiplier"), UGP_AttributeSet::GetCritMultiplierAttribute(), ASC);
	AddAttributeRow(TEXT("Lifesteal"), UGP_AttributeSet::GetLifestealAttribute(), ASC);
	AddAttributeRow(TEXT("DMG Increase %"), UGP_AttributeSet::GetDamageIncreaseRateAttribute(), ASC);

	AddAttributeRow(TEXT("--- Defensive ---"), FGameplayAttribute(), nullptr);
	AddAttributeRow(TEXT("Toughness"), UGP_AttributeSet::GetToughnessAttribute(), ASC);
	AddAttributeRow(TEXT("Max Toughness"), UGP_AttributeSet::GetMaxToughnessAttribute(), ASC);
	AddAttributeRow(TEXT("Toughness Recov"), UGP_AttributeSet::GetToughnessRecoveryRateAttribute(), ASC);
	
	AddAttributeRow(TEXT("--- Resistance ---"), FGameplayAttribute(), nullptr);
	AddAttributeRow(TEXT("Pyros Res"), UGP_AttributeSet::GetPyrosResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Hydro Res"), UGP_AttributeSet::GetHydroResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Volt Res"), UGP_AttributeSet::GetVoltResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Aero Res"), UGP_AttributeSet::GetAeroResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Lux Res"), UGP_AttributeSet::GetLuxResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Chaos Res"), UGP_AttributeSet::GetChaosResistanceAttribute(), ASC);
	AddAttributeRow(TEXT("Brute Res"), UGP_AttributeSet::GetBruteResistanceAttribute(), ASC);

	AddAttributeRow(TEXT("--- Utility ---"), FGameplayAttribute(), nullptr);
	AddAttributeRow(TEXT("Move Speed"), UGP_AttributeSet::GetMoveSpeedAttribute(), ASC);
}

void UGP_DebugAttributeWidget::AddAttributeRow(const FString& Label, const FGameplayAttribute& Attribute, UAbilitySystemComponent* ASC)
{
	UGP_DebugAttributeRow* NewRow = CreateWidget<UGP_DebugAttributeRow>(this, RowWidgetClass);
	if (NewRow)
	{
		NewRow->InitializeRow(Label, Attribute, ASC);
		AttributeContainer->AddChildToVerticalBox(NewRow);
	}
}
