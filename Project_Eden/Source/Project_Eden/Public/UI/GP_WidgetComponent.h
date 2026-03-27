#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "GP_WidgetComponent.generated.h"


class UAbilitySystemComponent;
class UAttributeSet;
class UGP_AttributeSet;
class UGP_AbilitySystemComponent;
class AGP_BaseCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:
protected:
	virtual void BeginPlay() override;

private:
	TWeakObjectPtr<AGP_BaseCharacter> GASCharacter;
	TWeakObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UGP_AttributeSet> AttributeSet;

	void InitAbilitySystemData();
	bool IsASCInitialized() const;

	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
};
