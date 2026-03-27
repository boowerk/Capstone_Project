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
protected:
	virtual void BeginPlay() override;
	UPROPERTY(EditAnywhere)
    	TMap<FGameplayAttribute, FGameplayAttribute> AttributeMap;

private:
	TWeakObjectPtr<AGP_BaseCharacter> GASCharacter;
	TWeakObjectPtr<UGP_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UGP_AttributeSet> AttributeSet;

	void InitAbilitySystemData();
	bool IsASCInitialized() const;
	void InitializeAttributeDelegate();

	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UFUNCTION()
	void BindToAttributeChanges();
	void BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;
};
