#include "UI/GP_PlayerHUDWidget.h"
#include "Components/Image.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GP_BaseCharacter.h"
#include "Characters/GP_EnemyCharacter.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/GP_MinimapSubsystem.h"
#include "UI/GP_SkillSlotHUDWidget.h"
#include "Player/GP_PlayerState.h"
#include "AbilitySystem/Abilities/GP_SkillData.h"

namespace
{
	bool IsLikelyMinimapMapImageName(const FString& WidgetName)
	{
		const bool bMentionsMinimap = WidgetName.Contains(TEXT("Minimap"), ESearchCase::IgnoreCase);
		const bool bMentionsMapBackground =
			WidgetName.Contains(TEXT("Background"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("MapImage"), ESearchCase::IgnoreCase);
		const bool bLooksLikeOverlayOnly =
			WidgetName.Contains(TEXT("Arrow"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Marker"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Point"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Ring"), ESearchCase::IgnoreCase)
			|| WidgetName.Contains(TEXT("Backplate"), ESearchCase::IgnoreCase);

		// Keep the fallback conservative: only the actual map image should receive the render target.
		return bMentionsMinimap && bMentionsMapBackground && !bLooksLikeOverlayOnly;
	}
}

UGP_PlayerHUDWidget::UGP_PlayerHUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LocationText = FText::FromString(TEXT("LIMINAL ASHEN FIELD"));
	BossText = FText::FromString(TEXT("Omen of the Drowned Belfry"));

	// The red point texture is stable project content; the generated UI material remains an explicit HUD setting.
	static ConstructorHelpers::FObjectFinder<UTexture2D> EnemyMarkerFinder(
		TEXT("/Game/UI/HUD/Minimap/Textures/T_UI_Minimap_Point_Red.T_UI_Minimap_Point_Red"));
	if (EnemyMarkerFinder.Succeeded())
	{
		MinimapEnemyMarkerTexture = EnemyMarkerFinder.Object;
	}

	// Provide the generated static-map material by default so the minimap works even if the BP field is left empty.
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> MinimapMaterialFinder(
		TEXT("/Game/UI/HUD/Minimap/Materials/M_UI_Minimap_StaticMap.M_UI_Minimap_StaticMap"));
	if (MinimapMaterialFinder.Succeeded())
	{
		MinimapMapMaterial = MinimapMaterialFinder.Object;
	}
}

void UGP_PlayerHUDWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshPreview();
}

void UGP_PlayerHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RefreshPreview();
	RefreshMinimapPlayerArrowRotation();
	BindToMinimapSubsystem();
	RefreshMinimapBackgroundFromSubsystem();
	RefreshMinimapPresentation(0.0f);

	// 1. 즉시 시도
	APawn* OwningPawn = GetOwningPlayerPawn();
	if (AGP_BaseCharacter* BaseChar = Cast<AGP_BaseCharacter>(OwningPawn))
	{
		if (UAbilitySystemComponent* ASC = BaseChar->GetAbilitySystemComponent())
		{
			BindToASC(ASC);
		}
		BaseChar->OnASCInitialized.RemoveAll(this);
		BaseChar->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitializedCallback);
	}
	else
	{
		// 2. 아직 Pawn이 없다면, 타이머로 잠시 후 다시 시도 (클라이언트 초기화 지연 대응)
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
			NativeConstruct();
		});
	}
}

void UGP_PlayerHUDWidget::NativeDestruct()
{
	if (BoundMinimapSubsystem.IsValid())
	{
		BoundMinimapSubsystem->OnRenderTargetChanged.RemoveDynamic(this, &ThisClass::HandleMinimapRenderTargetChanged);
	}

	BoundMinimapSubsystem.Reset();
	EnemyMarkerPool.Reset();
	MinimapMarkerCanvas = nullptr;
	MinimapMaterialInstance = nullptr;
	Super::NativeDestruct();
}

void UGP_PlayerHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	// Controller tick owns the marker interval; widget tick only supplies a zero-cost fallback for UV/rotation.
	RefreshMinimapPresentation(0.0f);
	if (!BoundMinimapRenderTarget.IsValid())
	{
		RefreshMinimapBackgroundFromSubsystem();
	}
}

void UGP_PlayerHUDWidget::SetMinimapRenderTarget(UTextureRenderTarget2D* InRenderTarget)
{
	UImage* BackgroundImage = ResolveMinimapBackgroundImage();
	if (!IsValid(BackgroundImage) || !IsValid(InRenderTarget))
	{
		return;
	}
	UObject* ExpectedBrushResource = IsValid(MinimapMaterialInstance)
		? static_cast<UObject*>(MinimapMaterialInstance.Get())
		: static_cast<UObject*>(InRenderTarget);
	if (BoundMinimapRenderTarget.Get() == InRenderTarget
		&& BackgroundImage->GetBrush().GetResourceObject() == ExpectedBrushResource)
	{
		return;
	}

	BoundMinimapRenderTarget = InRenderTarget;
	EnsureMinimapMaterial(InRenderTarget);
}

void UGP_PlayerHUDWidget::EnsureMinimapMaterial(UTextureRenderTarget2D* InRenderTarget)
{
	UImage* BackgroundImage = ResolveMinimapBackgroundImage();
	if (!IsValid(BackgroundImage) || !IsValid(InRenderTarget))
	{
		return;
	}

	if (!IsValid(MinimapMaterialInstance) && IsValid(MinimapMapMaterial))
	{
		MinimapMaterialInstance = UMaterialInstanceDynamic::Create(MinimapMapMaterial, this);
	}

	FSlateBrush Brush = BackgroundImage->GetBrush();
	if (IsValid(MinimapMaterialInstance))
	{
		// MapTexture never changes after initialization; only UV center and zoom are updated during play.
		MinimapMaterialInstance->SetTextureParameterValue(TEXT("MapTexture"), InRenderTarget);
		MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapZoom"), MinimapZoom);
		Brush.SetResourceObject(MinimapMaterialInstance);
	}
	else
	{
		// Direct render-target display remains a safe fallback until the UI material is assigned.
		Brush.SetResourceObject(InRenderTarget);
	}

	// Preserve the designer-authored brush size so the 1024px map texture cannot reflow the HUD layout.
	BackgroundImage->SetBrush(Brush);
}

void UGP_PlayerHUDWidget::RefreshMinimapPresentation(float DeltaSeconds)
{
	RefreshMinimapPlayerArrowRotation();
	RefreshMinimapMapUV();
	RefreshEnemyMinimapMarkers(DeltaSeconds);
}

void UGP_PlayerHUDWidget::RefreshMinimapMapUV()
{
	if (!IsValid(MinimapMaterialInstance) || !BoundMinimapSubsystem.IsValid())
	{
		return;
	}

	const APawn* OwningPawn = GetOwningPlayerPawn();
	FVector2D PlayerMapUV;
	if (!IsValid(OwningPawn) || !BoundMinimapSubsystem->WorldToMapUV(OwningPawn->GetActorLocation(), PlayerMapUV))
	{
		return;
	}

	if (PlayerMapUV.Equals(CachedPlayerMapUV, 0.0001f) && FMath::IsNearlyEqual(CachedMinimapZoom, MinimapZoom))
	{
		return;
	}

	// The material implements SourceUV = (WidgetUV - 0.5) / Zoom + PlayerMapUV.
	MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapCenterU"), PlayerMapUV.X);
	MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapCenterV"), PlayerMapUV.Y);
	MinimapMaterialInstance->SetScalarParameterValue(TEXT("MapZoom"), MinimapZoom);
	CachedPlayerMapUV = PlayerMapUV;
	CachedMinimapZoom = MinimapZoom;
}

void UGP_PlayerHUDWidget::RefreshMinimapBackgroundFromSubsystem()
{
	BindToMinimapSubsystem();

	if (BoundMinimapSubsystem.IsValid())
	{
		SetMinimapRenderTarget(BoundMinimapSubsystem->GetMinimapRenderTarget());
	}
}

void UGP_PlayerHUDWidget::BindToMinimapSubsystem()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UGP_MinimapSubsystem* MinimapSubsystem = World->GetSubsystem<UGP_MinimapSubsystem>();
	if (!IsValid(MinimapSubsystem) || BoundMinimapSubsystem.Get() == MinimapSubsystem)
	{
		return;
	}

	if (BoundMinimapSubsystem.IsValid())
	{
		BoundMinimapSubsystem->OnRenderTargetChanged.RemoveDynamic(this, &ThisClass::HandleMinimapRenderTargetChanged);
	}

	BoundMinimapSubsystem = MinimapSubsystem;
	MinimapSubsystem->OnRenderTargetChanged.AddUniqueDynamic(this, &ThisClass::HandleMinimapRenderTargetChanged);
}

UImage* UGP_PlayerHUDWidget::ResolveMinimapBackgroundImage() const
{
	if (IsValid(MinimapBackgroundImage))
	{
		return MinimapBackgroundImage.Get();
	}

	static const FName CandidateNames[] =
	{
		TEXT("MinimapBackgroundImage"),
		TEXT("MiniMapBackgroundImage"),
		TEXT("MinimapBackground"),
		TEXT("MiniMapBackground"),
		TEXT("MinimapImage"),
		TEXT("MiniMapImage"),
		TEXT("MinimapMapImage"),
		TEXT("MiniMapMapImage"),
		TEXT("MinimapRenderTargetImage"),
		TEXT("MiniMapRenderTargetImage"),
		TEXT("MapBackgroundImage"),
		TEXT("MapBackground"),
		TEXT("MapImage")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UImage* CandidateImage = Cast<UImage>(GetWidgetFromName(CandidateName)))
		{
			return CandidateImage;
		}
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	UImage* FoundImage = nullptr;
	WidgetTree->ForEachWidget([&FoundImage](UWidget* ChildWidget)
	{
		if (FoundImage || !ChildWidget)
		{
			return;
		}

		const FString WidgetName = ChildWidget->GetName();
		if (IsLikelyMinimapMapImageName(WidgetName))
		{
			FoundImage = Cast<UImage>(ChildWidget);
		}
	});

	return FoundImage;
}

bool UGP_PlayerHUDWidget::EnsureMinimapMarkerLayer()
{
	if (IsValid(MinimapMarkerCanvas))
	{
		return true;
	}
	if (UCanvasPanel* AuthoredMarkerCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("RuntimeMinimapMarkerCanvas"))))
	{
		MinimapMarkerCanvas = AuthoredMarkerCanvas;
		return true;
	}

	UImage* BackgroundImage = ResolveMinimapBackgroundImage();
	UPanelWidget* ParentPanel = IsValid(BackgroundImage) ? BackgroundImage->GetParent() : nullptr;
	if (!WidgetTree || !IsValid(ParentPanel))
	{
		return false;
	}

	MinimapMarkerCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("RuntimeMinimapMarkerCanvas"));
	if (!IsValid(MinimapMarkerCanvas))
	{
		return false;
	}

	MinimapMarkerCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
	MinimapMarkerCanvas->SetClipping(EWidgetClipping::ClipToBoundsAlways);

	if (UCanvasPanel* ParentCanvas = Cast<UCanvasPanel>(ParentPanel))
	{
		UCanvasPanelSlot* MarkerLayerSlot = ParentCanvas->AddChildToCanvas(MinimapMarkerCanvas);
		const UCanvasPanelSlot* BackgroundSlot = Cast<UCanvasPanelSlot>(BackgroundImage->Slot);
		if (!MarkerLayerSlot || !BackgroundSlot)
		{
			ParentCanvas->RemoveChild(MinimapMarkerCanvas);
			MinimapMarkerCanvas = nullptr;
			return false;
		}

		// Clone the authored map Image layout so markers share its exact pixel space at every resolution.
		MarkerLayerSlot->SetAnchors(BackgroundSlot->GetAnchors());
		MarkerLayerSlot->SetOffsets(BackgroundSlot->GetOffsets());
		MarkerLayerSlot->SetAlignment(BackgroundSlot->GetAlignment());
		MarkerLayerSlot->SetAutoSize(BackgroundSlot->GetAutoSize());
		MarkerLayerSlot->SetZOrder(BackgroundSlot->GetZOrder() + 1);
		return true;
	}

	if (UOverlay* ParentOverlay = Cast<UOverlay>(ParentPanel))
	{
		UOverlaySlot* MarkerLayerSlot = ParentOverlay->AddChildToOverlay(MinimapMarkerCanvas);
		const UOverlaySlot* BackgroundSlot = Cast<UOverlaySlot>(BackgroundImage->Slot);
		if (!MarkerLayerSlot || !BackgroundSlot)
		{
			ParentOverlay->RemoveChild(MinimapMarkerCanvas);
			MinimapMarkerCanvas = nullptr;
			return false;
		}

		MarkerLayerSlot->SetPadding(BackgroundSlot->GetPadding());
		MarkerLayerSlot->SetHorizontalAlignment(BackgroundSlot->GetHorizontalAlignment());
		MarkerLayerSlot->SetVerticalAlignment(BackgroundSlot->GetVerticalAlignment());
		return true;
	}

	// The shipped HUD uses Canvas/Overlay. A custom HUD can expose its own layer by naming it RuntimeMinimapMarkerCanvas.
	MinimapMarkerCanvas = nullptr;
	return false;
}

void UGP_PlayerHUDWidget::RefreshEnemyMinimapMarkers(float DeltaSeconds)
{
	EnemyMarkerRefreshAccumulator += FMath::Max(0.0f, DeltaSeconds);
	if (EnemyMarkerRefreshAccumulator < MinimapEnemyRefreshInterval)
	{
		return;
	}
	EnemyMarkerRefreshAccumulator = 0.0f;

	if (!BoundMinimapSubsystem.IsValid()
		|| !BoundMinimapSubsystem->IsMinimapReady()
		|| !IsValid(MinimapEnemyMarkerTexture)
		|| !EnsureMinimapMarkerLayer())
	{
		for (UImage* Marker : EnemyMarkerPool)
		{
			if (IsValid(Marker))
			{
				Marker->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		return;
	}

	const APawn* OwningPawn = GetOwningPlayerPawn();
	FVector2D PlayerMapUV;
	if (!IsValid(OwningPawn) || !BoundMinimapSubsystem->WorldToMapUV(OwningPawn->GetActorLocation(), PlayerMapUV))
	{
		return;
	}

	FVector2D MarkerLayerSize = MinimapMarkerCanvas->GetCachedGeometry().GetLocalSize();
	if (MarkerLayerSize.IsNearlyZero())
	{
		if (const UImage* BackgroundImage = ResolveMinimapBackgroundImage())
		{
			MarkerLayerSize = BackgroundImage->GetCachedGeometry().GetLocalSize();
		}
	}
	if (MarkerLayerSize.IsNearlyZero())
	{
		return;
	}

	int32 VisibleMarkerCount = 0;
	for (TActorIterator<AGP_EnemyCharacter> It(GetWorld()); It; ++It)
	{
		AGP_EnemyCharacter* Enemy = *It;
		if (!IsValid(Enemy) || Enemy->IsDead())
		{
			continue;
		}

		FVector2D EnemyMapUV;
		if (!BoundMinimapSubsystem->WorldToMapUV(Enemy->GetActorLocation(), EnemyMapUV))
		{
			continue;
		}

		const FVector2D ScreenUV = FVector2D(0.5f, 0.5f) + ((EnemyMapUV - PlayerMapUV) * MinimapZoom);
		if (FVector2D::Distance(ScreenUV, FVector2D(0.5f, 0.5f)) > MinimapMarkerVisibleRadius)
		{
			continue;
		}

		if (!EnemyMarkerPool.IsValidIndex(VisibleMarkerCount))
		{
			UImage* NewMarker = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				FName(*FString::Printf(TEXT("RuntimeEnemyMinimapMarker_%d"), VisibleMarkerCount)));
			if (!IsValid(NewMarker))
			{
				continue;
			}
			UCanvasPanelSlot* MarkerSlot = MinimapMarkerCanvas->AddChildToCanvas(NewMarker);
			if (!MarkerSlot)
			{
				continue;
			}

			// The requested red point texture is displayed as an independent UMG icon, never baked into the map.
			NewMarker->SetBrushFromTexture(MinimapEnemyMarkerTexture, true);
			NewMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
			MarkerSlot->SetAutoSize(false);
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetSize(FVector2D(MinimapEnemyMarkerSize));
			EnemyMarkerPool.Add(NewMarker);
		}

		UImage* Marker = EnemyMarkerPool[VisibleMarkerCount];
		if (IsValid(Marker))
		{
			Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (UCanvasPanelSlot* MarkerSlot = Cast<UCanvasPanelSlot>(Marker->Slot))
			{
				MarkerSlot->SetPosition(ScreenUV * MarkerLayerSize);
				MarkerSlot->SetSize(FVector2D(MinimapEnemyMarkerSize));
			}
		}
		++VisibleMarkerCount;
	}

	for (int32 Index = VisibleMarkerCount; Index < EnemyMarkerPool.Num(); ++Index)
	{
		if (IsValid(EnemyMarkerPool[Index]))
		{
			EnemyMarkerPool[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UGP_PlayerHUDWidget::RefreshMinimapPlayerArrowRotation()
{
	if (!bRotateMinimapPlayerArrow)
	{
		return;
	}

	APawn* OwningPawn = GetOwningPlayerPawn();
	UWidget* ArrowWidget = ResolveMinimapPlayerArrowWidget();
	if (!IsValid(OwningPawn) || !IsValid(ArrowWidget))
	{
		bHasCachedMinimapPlayerArrowAngle = false;
		return;
	}

	const float RotationSign = bInvertMinimapPlayerArrowRotation ? -1.0f : 1.0f;
	const float DesiredAngle = FRotator::NormalizeAxis((OwningPawn->GetActorRotation().Yaw * RotationSign) + MinimapPlayerArrowAngleOffset);

	// 같은 각도를 반복해서 쓰지 않도록 캐시해 Slate transform 갱신 비용을 줄입니다.
	if (bHasCachedMinimapPlayerArrowAngle && FMath::IsNearlyEqual(CachedMinimapPlayerArrowAngle, DesiredAngle, 0.1f))
	{
		return;
	}

	ArrowWidget->SetRenderTransformAngle(DesiredAngle);
	CachedMinimapPlayerArrowAngle = DesiredAngle;
	bHasCachedMinimapPlayerArrowAngle = true;
}

UWidget* UGP_PlayerHUDWidget::ResolveMinimapPlayerArrowWidget() const
{
	if (IsValid(MinimapPlayerArrow))
	{
		return MinimapPlayerArrow.Get();
	}

	static const FName CandidateNames[] =
	{
		TEXT("MinimapPlayerArrow"),
		TEXT("MiniMapPlayerArrow"),
		TEXT("MinimapPlayerArrowImage"),
		TEXT("MiniMapPlayerArrowImage"),
		TEXT("MinimapArrow"),
		TEXT("MiniMapArrow"),
		TEXT("PlayerArrow"),
		TEXT("PlayerArrowImage"),
		TEXT("PlayerDirectionArrow"),
		TEXT("MapPlayerArrow")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UWidget* CandidateWidget = GetWidgetFromName(CandidateName))
		{
			return CandidateWidget;
		}
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	UWidget* FoundArrowWidget = nullptr;
	WidgetTree->ForEachWidget([&FoundArrowWidget](UWidget* ChildWidget)
	{
		if (FoundArrowWidget || !ChildWidget)
		{
			return;
		}

		const FString WidgetName = ChildWidget->GetName();
		const bool bLooksLikeArrow = WidgetName.Contains(TEXT("Arrow"));
		const bool bLooksLikeMinimapPlayer =
			WidgetName.Contains(TEXT("Minimap")) ||
			WidgetName.Contains(TEXT("MiniMap")) ||
			WidgetName.Contains(TEXT("Player")) ||
			WidgetName.Contains(TEXT("Direction"));

		// 에디터에서 이름이 약간 달라도 미니맵 플레이어 방향 위젯을 최대한 안전하게 찾습니다.
		if (bLooksLikeArrow && bLooksLikeMinimapPlayer)
		{
			FoundArrowWidget = ChildWidget;
		}
	});

	if (FoundArrowWidget)
	{
		return FoundArrowWidget;
	}

	return nullptr;
}

void UGP_PlayerHUDWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC)) return;

	UGP_AttributeSet* AS = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AS)) return;

	RemoveAttributeDelegateHandles(BoundPlayerASC, PlayerAttributeDelegateHandles);
	BoundPlayerASC = InASC;

	auto ResolveAttributeWidget = [this](UGP_AttributeWidget* BoundWidget, const FName WidgetName)
	{
		if (IsValid(BoundWidget))
		{
			return BoundWidget;
		}

		return Cast<UGP_AttributeWidget>(GetWidgetFromName(WidgetName));
	};

	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(HealthBar, TEXT("HealthBar")), AS, PlayerAttributeDelegateHandles);
	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(ManaBar, TEXT("ManaBar")), AS, PlayerAttributeDelegateHandles);
	BindAttributeWidgetToASC(InASC, ResolveAttributeWidget(StaminaBar, TEXT("StaminaBar")), AS, PlayerAttributeDelegateHandles);
}

void UGP_PlayerHUDWidget::BindBossToASC(UAbilitySystemComponent* InASC)
{
	if (!IsValid(InASC))
	{
		ClearBossASC();
		return;
	}

	UGP_AttributeSet* AS = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AS))
	{
		ClearBossASC();
		return;
	}

	if (UGP_AttributeWidget* BossAttributeWidget = ResolveBossHealthBar())
	{
		EnsureBossHealthAttributes(BossAttributeWidget);

		if (BoundBossASC == InASC && BossAttributeDelegateHandles.Num() > 0)
		{
			TTuple<FGameplayAttribute, FGameplayAttribute> Pair(BossAttributeWidget->Attribute, BossAttributeWidget->MaxAttribute);
			if (Pair.Key.IsValid() && Pair.Value.IsValid())
			{
				BossAttributeWidget->OnAttributeChange(Pair, AS);
			}
			return;
		}

		RemoveAttributeDelegateHandles(BoundBossASC, BossAttributeDelegateHandles);
		BoundBossASC = InASC;
		BindAttributeWidgetToASC(InASC, BossAttributeWidget, AS, BossAttributeDelegateHandles);
		return;
	}

	ClearBossASC();
}

void UGP_PlayerHUDWidget::ClearBossASC()
{
	RemoveAttributeDelegateHandles(BoundBossASC, BossAttributeDelegateHandles);
}

void UGP_PlayerHUDWidget::BindAttributeWidgetToASC(UAbilitySystemComponent* InASC, UGP_AttributeWidget* Widget, UGP_AttributeSet* AttributeSet, TArray<FGPAttributeDelegateBinding>& DelegateHandles)
{
	if (!IsValid(InASC) || !IsValid(Widget) || !IsValid(AttributeSet))
	{
		return;
	}

	TTuple<FGameplayAttribute, FGameplayAttribute> Pair(Widget->Attribute, Widget->MaxAttribute);
	if (!Pair.Key.IsValid() || !Pair.Value.IsValid())
	{
		return;
	}

	Widget->OnAttributeChange(Pair, AttributeSet);

	TWeakObjectPtr<UGP_AttributeWidget> WeakWidget(Widget);
	TWeakObjectPtr<UGP_AttributeSet> WeakAS(AttributeSet);

	FDelegateHandle CurrentValueHandle = InASC->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
	{
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
	DelegateHandles.Emplace(Pair.Key, CurrentValueHandle);

	FDelegateHandle MaxValueHandle = InASC->GetGameplayAttributeValueChangeDelegate(Pair.Value).AddLambda([WeakWidget, Pair, WeakAS](const FOnAttributeChangeData& Data)
	{
		if (WeakWidget.IsValid() && WeakAS.IsValid())
		{
			WeakWidget->OnAttributeChange(Pair, WeakAS.Get());
		}
	});
	DelegateHandles.Emplace(Pair.Value, MaxValueHandle);
}

void UGP_PlayerHUDWidget::RemoveAttributeDelegateHandles(TWeakObjectPtr<UAbilitySystemComponent>& BoundASC, TArray<FGPAttributeDelegateBinding>& DelegateHandles)
{
	if (BoundASC.IsValid())
	{
		for (const FGPAttributeDelegateBinding& Binding : DelegateHandles)
		{
			if (Binding.Attribute.IsValid() && Binding.Handle.IsValid())
			{
				// Remove the handle from the exact GAS attribute delegate it was added to.
				BoundASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.Handle);
			}
		}
	}

	DelegateHandles.Reset();
	BoundASC.Reset();
}

void UGP_PlayerHUDWidget::EnsureBossHealthAttributes(UGP_AttributeWidget* Widget) const
{
	if (!IsValid(Widget))
	{
		return;
	}

	// WBP_BossBar is a GP_AttributeWidget, so force it to boss Health/MaxHealth even if the BP defaults are missing.
	Widget->Attribute = UGP_AttributeSet::GetHealthAttribute();
	Widget->MaxAttribute = UGP_AttributeSet::GetMaxHealthAttribute();
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveAttributeWidgetFromWidget(UWidget* WidgetObject) const
{
	if (!IsValid(WidgetObject))
	{
		return nullptr;
	}

	if (UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(WidgetObject))
	{
		return AttributeWidget;
	}

	const UUserWidget* UserWidget = Cast<UUserWidget>(WidgetObject);
	if (!IsValid(UserWidget) || !UserWidget->WidgetTree)
	{
		return nullptr;
	}

	UGP_AttributeWidget* FoundWidget = nullptr;
	UserWidget->WidgetTree->ForEachWidget([&FoundWidget](UWidget* ChildWidget)
	{
		if (!FoundWidget)
		{
			FoundWidget = Cast<UGP_AttributeWidget>(ChildWidget);
		}
	});

	return FoundWidget;
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveAttributeWidgetByName(const FName WidgetName) const
{
	// Boss bars may be either a direct GP_AttributeWidget or a wrapper UserWidget that contains one.
	return ResolveAttributeWidgetFromWidget(GetWidgetFromName(WidgetName));
}

UGP_AttributeWidget* UGP_PlayerHUDWidget::ResolveBossHealthBar() const
{
	if (BossHealthBar)
	{
		return BossHealthBar;
	}

	if (!WidgetTree)
	{
		return nullptr;
	}

	static const FName CandidateNames[] =
	{
		TEXT("BossHealthBar"),
		TEXT("BossBar"),
		TEXT("WBP_BossBar"),
		TEXT("BossHealth"),
		TEXT("BossHPBar")
	};

	for (const FName& CandidateName : CandidateNames)
	{
		if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetByName(CandidateName))
		{
			return CandidateWidget;
		}
	}

	if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetFromWidget(BossFrame))
	{
		return CandidateWidget;
	}

	if (UGP_AttributeWidget* CandidateWidget = ResolveAttributeWidgetByName(TEXT("BossBox")))
	{
		return CandidateWidget;
	}

	UGP_AttributeWidget* FoundBossWidget = nullptr;
	WidgetTree->ForEachWidget([this, &FoundBossWidget](UWidget* ChildWidget)
	{
		if (!FoundBossWidget && ChildWidget && ChildWidget->GetName().Contains(TEXT("Boss")))
		{
			// As a last resort, scan only Boss-named widgets to avoid binding the player health bar by mistake.
			FoundBossWidget = ResolveAttributeWidgetFromWidget(ChildWidget);
		}
	});

	if (FoundBossWidget)
	{
		return FoundBossWidget;
	}

	return nullptr;
}

void UGP_PlayerHUDWidget::SetLocationText(const FText& InLocationText)
{
	LocationText = InLocationText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossText(const FText& InBossText)
{
	BossText = InBossText;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::SetBossVisible(bool bIsVisible)
{
	bShowBossFrame = bIsVisible;
	RefreshPreview();
}

void UGP_PlayerHUDWidget::RefreshPreview()
{
	if (LocationTextBlock) LocationTextBlock->SetText(LocationText);
	if (BossTextBlock) BossTextBlock->SetText(BossText);
	if (BossFrame) BossFrame->SetVisibility(bShowBossFrame ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}

void UGP_PlayerHUDWidget::OnASCInitializedCallback(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	BindToASC(ASC);

	// On a dedicated-server client the ASC (and equipped skill data) arrive after
	// the controller's BeginPlay binding attempt, so re-bind the skill slots here
	// once the ASC is ready. Otherwise the slot icons stay empty even though the
	// skill list (which reads the always-loaded skill pool) shows them fine.
	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (AGP_PlayerState* GPPS = OwningPC->GetPlayerState<AGP_PlayerState>())
		{
			BindSkillSlots(GPPS);
		}
	}
}

void UGP_PlayerHUDWidget::HandleMinimapRenderTargetChanged(UTextureRenderTarget2D* InRenderTarget)
{
	SetMinimapRenderTarget(InRenderTarget);
}

void UGP_PlayerHUDWidget::BindSkillSlots(AGP_PlayerState* PS)
{
	if (!IsValid(PS))
	{
		return;
	}

	// The icon only needs the (replicated) equipped SkillData from the PlayerState;
	// the ASC is just for cooldown polling and may still be null on a dedicated
	// client. Bind regardless so icons appear, and rely on OnEquippedSkillChanged /
	// the ASC rebind to fill in cooldowns once it arrives.
	UAbilitySystemComponent* ASC = BoundPlayerASC.Get();

	// Slot tags match the convention used in GP_PlayerState (Slot01, Slot02).
	static const FGameplayTag Slot1Tag = FGameplayTag::RequestGameplayTag(TEXT("GPTags.Ability.Skill.Slot01"), false);
	static const FGameplayTag Slot2Tag = FGameplayTag::RequestGameplayTag(TEXT("GPTags.Ability.Skill.Slot02"), false);

	if (SkillSlot1)
	{
		UGP_SkillData* Data = PS->GetEquippedSkillData(Slot1Tag);
		SkillSlot1->SetupSlot(ASC, Data, FText::FromString(TEXT("Q")));
	}

	if (SkillSlot2)
	{
		UGP_SkillData* Data = PS->GetEquippedSkillData(Slot2Tag);
		SkillSlot2->SetupSlot(ASC, Data, FText::FromString(TEXT("E")));
	}

	// Re-bind whenever the player swaps skills.
	BoundSkillPlayerState = PS;
	PS->OnEquippedSkillChanged.AddUniqueDynamic(this, &UGP_PlayerHUDWidget::OnEquippedSkillChanged);
}

void UGP_PlayerHUDWidget::OnEquippedSkillChanged(FGameplayTag /*SlotTag*/, UGP_SkillData* /*SkillData*/)
{
	if (BoundSkillPlayerState.IsValid())
	{
		BindSkillSlots(BoundSkillPlayerState.Get());
	}
}

void UGP_PlayerHUDWidget::EnsureSkillSlotsBound()
{
	// Once BindSkillSlots succeeds it caches BoundSkillPlayerState and subscribes
	// to OnEquippedSkillChanged, so later equips are handled by the delegate — no
	// need to keep retrying.
	if (BoundSkillPlayerState.IsValid())
	{
		return;
	}

	if (APlayerController* OwningPC = GetOwningPlayer())
	{
		if (AGP_PlayerState* GPPS = OwningPC->GetPlayerState<AGP_PlayerState>())
		{
			BindSkillSlots(GPPS);
		}
	}
}
