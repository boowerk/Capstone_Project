// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/GP_AttributeSet.h"

#include "GP_AttributeWidget.generated.h"

class UProgressBar;

/**
 * 
 */
UCLASS()
class PROJECT_EDEN_API UGP_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()
public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Attributes")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Attributes")
	FGameplayAttribute MaxAttribute;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Attributes")
	bool bHideWhenFull = false;

	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UGP_AttributeSet* AttributeSet);
	bool MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChange(float NewValue, float NewMaxValue);
	
protected:
	virtual void NativeConstruct() override;

private:
	// Blueprint가 BP_OnAttributeChange를 구현하지 않아도 기본 ProgressBar를 GAS 값으로 갱신하기 위한 fallback입니다.
	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> CachedNativeProgressBar;

	void ApplyNativeProgressBarValue(float NewValue, float NewMaxValue);
	UProgressBar* ResolveNativeProgressBar();
	bool ShouldUseBossFillColorFallback() const;
};
