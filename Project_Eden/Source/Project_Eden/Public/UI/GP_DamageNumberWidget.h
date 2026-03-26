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
    virtual void NativeConstruct() override;

    void SetDamageData(int32 InDamageValue, EWeaponElement InElement);
    void SetDisplayOpacity(float InOpacity);
    void SetDisplayScale(float InScale);

protected:
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> DamageTextBlock;

private:
    void EnsureWidgetTree();
    FLinearColor GetElementColor(EWeaponElement InElement) const;
};
