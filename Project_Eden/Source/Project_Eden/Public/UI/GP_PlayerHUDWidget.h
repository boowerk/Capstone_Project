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

UCLASS()
class PROJECT_EDEN_API UGP_PlayerHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer);

	// ASC를 받아서 델리게이트를 연결할 함수
	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|GAS")
	void BindToASC(UAbilitySystemComponent* InASC);

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
	
	UFUNCTION()
	void OnASCInitializedCallback(class UAbilitySystemComponent* ASC, class UAttributeSet* AS);
	
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> HealthBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> ManaBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UGP_AttributeWidget> StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> LocationTextBlock;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> BossTextBlock;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "HUD", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> BossFrame;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText LocationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText BossText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	bool bShowBossFrame = false;
};
