#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"
#include "AttributeSet.h"

#include "GP_WidgetComponent.generated.h"


class UAbilitySystemComponent;
class UGP_AttributeSet;
class UGP_AbilitySystemComponent;
class AGP_BaseCharacter;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECT_EDEN_API UGP_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
public:
	UGP_WidgetComponent();

	// Tests and owning actors can verify the GAS contract without exposing the editable map itself.
	bool TracksAttributePair(const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute) const;

protected:
	virtual void BeginPlay() override;

	// Maps a displayed attribute to its max attribute, for example Health -> MaxHealth.
	UPROPERTY(EditAnywhere, Category = "GAS|Attributes")
	TMap<FGameplayAttribute, FGameplayAttribute> AttributeMap;

	// Toggle this in the widget component details to verify that enemy health UI receives GAS updates.
	UPROPERTY(EditAnywhere, Category = "Debug|Health")
	bool bDebugAttributeChanges = false;

	UPROPERTY(EditAnywhere, Category = "Debug|Health", meta = (EditCondition = "bDebugAttributeChanges", ClampMin = "0.1", Units = "s"))
	float DebugAttributeMessageDuration = 1.5f;

private:
	TWeakObjectPtr<AGP_BaseCharacter> GASCharacter;
	TWeakObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UGP_AttributeSet> AttributeSet;
	bool bBoundAttributeChanges = false;

	void InitAbilitySystemData();
	bool IsASCInitialized() const;
	void InitializeAttributeDelegate();
	void DebugLogAttributeUpdate(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, float NewValue, float NewMaxValue) const;

	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UFUNCTION()
	void BindToAttributeChanges();
	bool BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;
};
