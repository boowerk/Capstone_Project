#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AttributeSet.h"
#include "GP_DebugAttributeWidget.generated.h"

class UVerticalBox;
class UGP_DebugAttributeRow;
class UAbilitySystemComponent;

UCLASS()
class PROJECT_EDEN_API UGP_DebugAttributeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Debug UI")
	void InitializeDebugWidget();

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> AttributeContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Debug UI")
	TSubclassOf<UGP_DebugAttributeRow> RowWidgetClass;

private:
	void AddAttributeRow(const FString& Label, const FGameplayAttribute& Attribute, UAbilitySystemComponent* ASC);
};
