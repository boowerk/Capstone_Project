#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Items/WeaponItemTypes.h"

#include "GP_DamageNumberWidget.generated.h"

class UTextBlock;

UCLASS()
class PROJECT_EDEN_API UGP_DamageNumberWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativePreConstruct() override;
    virtual void NativeConstruct() override;

    void SetDamageData(int32 InDamageValue, EWeaponElement InElement);
    void SetDisplayOpacity(float InOpacity);
    void SetDisplayScale(float InScale);

protected:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    TObjectPtr<UTextBlock> DamageTextBlock;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number|Preview", meta = (ClampMin = "0", UIMin = "0"))
    int32 PreviewDamageValue = 1234;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage Number|Preview")
    EWeaponElement PreviewElement = EWeaponElement::Fire;

private:
    void EnsureWidgetTree();
    void ApplyDamageData(int32 InDamageValue, EWeaponElement InElement);
    FLinearColor GetElementColor(EWeaponElement InElement) const;

    int32 CurrentDamageValue = 0;
    EWeaponElement CurrentElement = EWeaponElement::Fire;
};
