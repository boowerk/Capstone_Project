#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "GP_DebugAttributeRow.generated.h"

class UTextBlock;

UCLASS()
class PROJECT_EDEN_API UGP_DebugAttributeRow : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeRow(const FString& InLabel, const FGameplayAttribute& InAttribute, UAbilitySystemComponent* InASC);

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LabelText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ValueText;

	FGameplayAttribute TargetAttribute;
	
	void UpdateValue(const struct FOnAttributeChangeData& Data);
	void RefreshValue();
};
