#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h" // FOnAttributeChangeData 사용을 위해 필요
#include "GP_AttributeWidget.h"
#include "GP_PlayerHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UWidget;
class UAbilitySystemComponent;
class UGP_AttributeSet;

struct FGPAttributeDelegateBinding
{
	FGPAttributeDelegateBinding() = default;
	FGPAttributeDelegateBinding(const FGameplayAttribute& InAttribute, const FDelegateHandle& InHandle)
		: Attribute(InAttribute)
		, Handle(InHandle)
	{
	}

	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

UCLASS()
class PROJECT_EDEN_API UGP_PlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer);

	// ASC를 받아서 델리게이트를 연결할 함수
	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|GAS")
	void BindToASC(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|GAS")
	void BindBossToASC(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|GAS")
	void ClearBossASC();

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetLocationText(const FText& InLocationText);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetBossText(const FText& InBossText);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD")
	void SetBossVisible(bool bIsVisible);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

private:
	void RefreshPreview();
	void BindAttributeWidgetToASC(UAbilitySystemComponent* InASC, UGP_AttributeWidget* Widget, UGP_AttributeSet* AttributeSet, TArray<FGPAttributeDelegateBinding>& DelegateHandles);
	void RemoveAttributeDelegateHandles(TWeakObjectPtr<UAbilitySystemComponent>& BoundASC, TArray<FGPAttributeDelegateBinding>& DelegateHandles);
	void EnsureBossHealthAttributes(UGP_AttributeWidget* Widget) const;
	UGP_AttributeWidget* ResolveAttributeWidgetFromWidget(UWidget* WidgetObject) const;
	UGP_AttributeWidget* ResolveAttributeWidgetByName(const FName WidgetName) const;
	UGP_AttributeWidget* ResolveBossHealthBar() const;
	
	UFUNCTION()
	void OnASCInitializedCallback(class UAbilitySystemComponent* ASC, class UAttributeSet* AS);
	
	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> HealthBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> ManaBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> StaminaBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> BossHealthBar;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> LocationTextBlock;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> BossTextBlock;

	UPROPERTY(BlueprintReadOnly, Category = "HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> BossFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText LocationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText BossText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	bool bShowBossFrame = false;

	TWeakObjectPtr<UAbilitySystemComponent> BoundPlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> BoundBossASC;
	TArray<FGPAttributeDelegateBinding> PlayerAttributeDelegateHandles;
	TArray<FGPAttributeDelegateBinding> BossAttributeDelegateHandles;
};
