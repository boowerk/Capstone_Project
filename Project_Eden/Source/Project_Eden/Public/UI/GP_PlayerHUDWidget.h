#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h" // FOnAttributeChangeData 사용을 위해 필요
#include "GP_AttributeWidget.h"
#include "GP_PlayerHUDWidget.generated.h"

class UTextBlock;
class UProgressBar;
class UWidget;
class UImage;
class UTextureRenderTarget2D;
class UTexture2D;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UCanvasPanel;
class UAbilitySystemComponent;
class UGP_AttributeSet;
class UGP_MinimapSubsystem;

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

	// PlayerController tick에서도 호출해 UUserWidget native tick 설정과 무관하게 미니맵 방향을 갱신합니다.
	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|Minimap")
	void RefreshMinimapPlayerArrowRotation();

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|Minimap")
	void SetMinimapRenderTarget(UTextureRenderTarget2D* InRenderTarget);

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|Minimap")
	void RefreshMinimapBackgroundFromSubsystem();

	UFUNCTION(BlueprintCallable, Category = "EldenRing HUD|Minimap")
	void RefreshMinimapPresentation(float DeltaSeconds = 0.0f);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void RefreshPreview();
	void BindToMinimapSubsystem();
	UImage* ResolveMinimapBackgroundImage() const;
	UWidget* ResolveMinimapPlayerArrowWidget() const;
	void EnsureMinimapMaterial(UTextureRenderTarget2D* InRenderTarget);
	void RefreshMinimapMapUV();
	bool EnsureMinimapMarkerLayer();
	void RefreshEnemyMinimapMarkers(float DeltaSeconds);
	void BindAttributeWidgetToASC(UAbilitySystemComponent* InASC, UGP_AttributeWidget* Widget, UGP_AttributeSet* AttributeSet, TArray<FGPAttributeDelegateBinding>& DelegateHandles);
	void RemoveAttributeDelegateHandles(TWeakObjectPtr<UAbilitySystemComponent>& BoundASC, TArray<FGPAttributeDelegateBinding>& DelegateHandles);
	void EnsureBossHealthAttributes(UGP_AttributeWidget* Widget) const;
	UGP_AttributeWidget* ResolveAttributeWidgetFromWidget(UWidget* WidgetObject) const;
	UGP_AttributeWidget* ResolveAttributeWidgetByName(const FName WidgetName) const;
	UGP_AttributeWidget* ResolveBossHealthBar() const;
	
	UFUNCTION()
	void OnASCInitializedCallback(class UAbilitySystemComponent* ASC, class UAttributeSet* AS);

	UFUNCTION()
	void HandleMinimapRenderTargetChanged(UTextureRenderTarget2D* InRenderTarget);
	
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

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Minimap", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> MinimapPlayerArrow;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Minimap", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> MinimapBackgroundImage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText LocationText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	FText BossText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Preview", meta = (AllowPrivateAccess = "true"))
	bool bShowBossFrame = false;

	// 미니맵 화살표는 에디터 배치 위젯을 유지하고, C++에서는 플레이어 yaw만 RenderTransformAngle에 반영합니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true"))
	bool bRotateMinimapPlayerArrow = true;

	// 화살표 이미지의 기본 방향이 프로젝트 기준과 다를 때 에디터에서 보정할 수 있는 각도입니다.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "-360.0", ClampMax = "360.0"))
	float MinimapPlayerArrowAngleOffset = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true"))
	bool bInvertMinimapPlayerArrowRotation = false;

	// UI material samples the one-shot full-map texture and pans/zooms it around the player's world-space UV.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> MinimapMapMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "1.0", ClampMax = "12.0"))
	float MinimapZoom = 3.0f;

	// Enemy markers are regular UMG Images, intentionally separate from the captured map texture.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTexture2D> MinimapEnemyMarkerTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "2.0", ClampMax = "64.0"))
	float MinimapEnemyMarkerSize = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.02", ClampMax = "1.0"))
	float MinimapEnemyRefreshInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EldenRing HUD|Minimap", meta = (AllowPrivateAccess = "true", ClampMin = "0.1", ClampMax = "0.5"))
	float MinimapMarkerVisibleRadius = 0.47f;

	TWeakObjectPtr<UAbilitySystemComponent> BoundPlayerASC;
	TWeakObjectPtr<UAbilitySystemComponent> BoundBossASC;
	TArray<FGPAttributeDelegateBinding> PlayerAttributeDelegateHandles;
	TArray<FGPAttributeDelegateBinding> BossAttributeDelegateHandles;
	bool bHasCachedMinimapPlayerArrowAngle = false;
	float CachedMinimapPlayerArrowAngle = 0.0f;
	TWeakObjectPtr<UGP_MinimapSubsystem> BoundMinimapSubsystem;
	TWeakObjectPtr<UTextureRenderTarget2D> BoundMinimapRenderTarget;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> MinimapMaterialInstance;
	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> MinimapMarkerCanvas;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> EnemyMarkerPool;
	FVector2D CachedPlayerMapUV = FVector2D(-1.0f, -1.0f);
	float CachedMinimapZoom = -1.0f;
	float EnemyMarkerRefreshAccumulator = 0.0f;
};
