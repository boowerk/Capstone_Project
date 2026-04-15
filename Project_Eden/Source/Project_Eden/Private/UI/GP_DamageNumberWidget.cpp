#include "UI/GP_DamageNumberWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"

void UGP_DamageNumberWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    EnsureWidgetTree();

    if (IsDesignTime())
    {
        ApplyDamageData(PreviewDamageValue, PreviewElement);
    }
}

void UGP_DamageNumberWidget::NativeConstruct()
{
    Super::NativeConstruct();
    EnsureWidgetTree();
    ApplyDamageData(CurrentDamageValue, CurrentElement);
}

void UGP_DamageNumberWidget::SetDamageData(int32 InDamageValue, EWeaponElement InElement)
{
    CurrentDamageValue = InDamageValue;
    CurrentElement = InElement;
    ApplyDamageData(InDamageValue, InElement);
}

void UGP_DamageNumberWidget::SetDisplayOpacity(float InOpacity)
{
    SetRenderOpacity(FMath::Clamp(InOpacity, 0.0f, 1.0f));
}

void UGP_DamageNumberWidget::SetDisplayScale(float InScale)
{
    SetRenderScale(FVector2D(InScale, InScale));
}

void UGP_DamageNumberWidget::EnsureWidgetTree()
{
    if (DamageTextBlock)
    {
        return;
    }

    if (WidgetTree)
    {
        DamageTextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DamageText")));
    }

    if (DamageTextBlock)
    {
        return;
    }

    if (!WidgetTree || WidgetTree->RootWidget)
    {
        return;
    }

    // 위젯 블루프린트를 아직 만들지 않은 경우에도 기본 텍스트 위젯으로 동작하게 둔다.
    DamageTextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DamageText"));
    WidgetTree->RootWidget = DamageTextBlock;

    FSlateFontInfo FontInfo = DamageTextBlock->GetFont();
    FontInfo.Size = 42;
    DamageTextBlock->SetFont(FontInfo);
    DamageTextBlock->SetJustification(ETextJustify::Center);
    DamageTextBlock->SetShadowOffset(FVector2D(2.0f, 3.0f));
    DamageTextBlock->SetShadowColorAndOpacity(FLinearColor(0.02f, 0.02f, 0.02f, 0.85f));
}

void UGP_DamageNumberWidget::ApplyDamageData(int32 InDamageValue, EWeaponElement InElement)
{
    EnsureWidgetTree();

    if (!DamageTextBlock)
    {
        return;
    }

    DamageTextBlock->SetText(FText::AsNumber(InDamageValue));
    DamageTextBlock->SetColorAndOpacity(FSlateColor(GetElementColor(InElement)));
}

FLinearColor UGP_DamageNumberWidget::GetElementColor(EWeaponElement InElement) const
{
    // 무기 원소 속성에 맞춰 데미지 숫자 색을 고정 매핑한다.
    switch (InElement)
    {
    case EWeaponElement::Fire:
        return FLinearColor(1.0f, 0.47f, 0.20f, 1.0f);
    case EWeaponElement::Water:
        return FLinearColor(0.34f, 0.80f, 1.0f, 1.0f);
    case EWeaponElement::Lightning:
        return FLinearColor(1.0f, 0.87f, 0.35f, 1.0f);
    default:
        return FLinearColor::White;
    }
}
