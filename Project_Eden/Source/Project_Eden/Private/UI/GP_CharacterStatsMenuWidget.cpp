#include "UI/GP_CharacterStatsMenuWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GP_BaseCharacter.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Components/WidgetSwitcher.h"
#include "InputCoreTypes.h"
#include "UI/GP_AttributeWidget.h"

UGP_CharacterStatsMenuWidget::UGP_CharacterStatsMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Designer widgets only need these names; C++ resolves and updates the value TextBlocks from GAS snapshots.
	StatTextBindings =
	{
		FGPCharacterStatTextBinding(FName(TEXT("Health")), FName(TEXT("HpValueText"))),
		FGPCharacterStatTextBinding(FName(TEXT("AttackPower")), FName(TEXT("AttackValueText"))),
		FGPCharacterStatTextBinding(FName(TEXT("Armor")), FName(TEXT("DefenseValueText"))),
		FGPCharacterStatTextBinding(FName(TEXT("Toughness")), FName(TEXT("StaggerValueText"))),
		FGPCharacterStatTextBinding(FName(TEXT("MagicPower")), FName(TEXT("MagicValueText"))),
		FGPCharacterStatTextBinding(FName(TEXT("MoveSpeed")), FName(TEXT("SpeedValueText")))
	};

	ContentSwitcherNames =
	{
		FName(TEXT("TabContentSwitcher")),
		FName(TEXT("MenuContentSwitcher")),
		FName(TEXT("ContentSwitcher")),
		FName(TEXT("CharacterMenuContentSwitcher"))
	};
}

void UGP_CharacterStatsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	RebuildStatTextBindings();
	RebuildNativeTabBindings();

	// Auto-bind from the owning pawn as a fallback when the PlayerController did not initialize the widget yet.
	if (!BoundCharacter.IsValid())
	{
		InitializeForCharacter(Cast<AGP_BaseCharacter>(GetOwningPlayerPawn()));
		return;
	}

	SetActiveTab(DefaultTab);
}

FReply UGP_CharacterStatsMenuWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bEnableNativeTabHitTesting && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		EGPCharacterMenuTab HitTab = ActiveTab;
		if (FindTabAtScreenPosition(InMouseEvent.GetScreenSpacePosition(), HitTab))
		{
			SetActiveTab(HitTab);
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UGP_CharacterStatsMenuWidget::NativeDestruct()
{
	if (BoundCharacter.IsValid())
	{
		BoundCharacter->OnASCInitialized.RemoveAll(this);
	}

	ClearASC();
	Super::NativeDestruct();
}

void UGP_CharacterStatsMenuWidget::InitializeForCharacter(AGP_BaseCharacter* InCharacter)
{
	if (BoundCharacter.IsValid())
	{
		// Remove the old pawn callback before rebinding so respawn/reopen does not duplicate ASC subscriptions.
		BoundCharacter->OnASCInitialized.RemoveAll(this);
	}

	BoundCharacter = InCharacter;
	SetActiveTab(DefaultTab);

	if (!IsValid(InCharacter))
	{
		ClearASC();
		BP_OnMenuBound(nullptr);
		return;
	}

	InCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitializedCallback);
	BindToASC(InCharacter->GetAbilitySystemComponent());
	BP_OnMenuBound(InCharacter);
}

void UGP_CharacterStatsMenuWidget::BindToASC(UAbilitySystemComponent* InASC)
{
	ClearASC();

	if (!IsValid(InASC))
	{
		return;
	}

	UGP_AttributeSet* AttributeSet = const_cast<UGP_AttributeSet*>(Cast<UGP_AttributeSet>(InASC->GetAttributeSet(UGP_AttributeSet::StaticClass())));
	if (!IsValid(AttributeSet))
	{
		return;
	}

	BoundASC = InASC;
	BoundAttributeSet = AttributeSet;

	TArray<FGameplayAttribute> AttributesToBind;
	AddSnapshotAttributes(AttributesToBind);
	AddWidgetTreeAttributes(AttributesToBind);

	for (const FGameplayAttribute& Attribute : AttributesToBind)
	{
		if (!Attribute.IsValid())
		{
			continue;
		}

		// A single GAS delegate refreshes both native TextBlocks and embedded GP_AttributeWidget bars.
		const FDelegateHandle Handle = InASC->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(
			this,
			&ThisClass::HandleBoundAttributeChanged);
		AttributeBindings.Emplace(Attribute, Handle);
	}

	RefreshAttributeSnapshots();
	RefreshAttributeWidgets();
}

void UGP_CharacterStatsMenuWidget::ClearASC()
{
	RemoveAttributeBindings();
	BoundASC.Reset();
	BoundAttributeSet.Reset();

	AttributeSnapshots.Reset();
	NotifyAttributeSnapshotsUpdated();
}

void UGP_CharacterStatsMenuWidget::SetActiveTab(EGPCharacterMenuTab NewTab)
{
	ActiveTab = NewTab;
	ApplyActiveTabState();
	OnActiveTabChanged.Broadcast(ActiveTab);
	BP_OnActiveTabChanged(ActiveTab);
}

void UGP_CharacterStatsMenuWidget::SelectMapTab()
{
	SetActiveTab(EGPCharacterMenuTab::Map);
}

void UGP_CharacterStatsMenuWidget::SelectJournalTab()
{
	SetActiveTab(EGPCharacterMenuTab::Journal);
}

void UGP_CharacterStatsMenuWidget::SelectItemsTab()
{
	SetActiveTab(EGPCharacterMenuTab::Items);
}

void UGP_CharacterStatsMenuWidget::SelectAttributesTab()
{
	SetActiveTab(EGPCharacterMenuTab::Attributes);
}

void UGP_CharacterStatsMenuWidget::SelectGearTab()
{
	SetActiveTab(EGPCharacterMenuTab::Gear);
}

void UGP_CharacterStatsMenuWidget::SelectAbilitiesTab()
{
	SetActiveTab(EGPCharacterMenuTab::Abilities);
}

void UGP_CharacterStatsMenuWidget::SelectSystemTab()
{
	SetActiveTab(EGPCharacterMenuTab::System);
}

void UGP_CharacterStatsMenuWidget::RefreshAttributeSnapshots()
{
	AttributeSnapshots.Reset();

	if (!BoundAttributeSet.IsValid())
	{
		NotifyAttributeSnapshotsUpdated();
		return;
	}

	// The menu exposes current GAS attributes as a compact view model for both native and optional BP UI.
	AddStatSnapshot(FName(TEXT("Health")), NSLOCTEXT("GPCharacterStatsMenu", "Health", "HP"), UGP_AttributeSet::GetHealthAttribute(), UGP_AttributeSet::GetMaxHealthAttribute());
	AddStatSnapshot(FName(TEXT("Mana")), NSLOCTEXT("GPCharacterStatsMenu", "Mana", "Mana"), UGP_AttributeSet::GetManaAttribute(), UGP_AttributeSet::GetMaxManaAttribute());
	AddStatSnapshot(FName(TEXT("AttackPower")), NSLOCTEXT("GPCharacterStatsMenu", "AttackPower", "Attack"), UGP_AttributeSet::GetAttackPowerAttribute());
	AddStatSnapshot(FName(TEXT("MagicPower")), NSLOCTEXT("GPCharacterStatsMenu", "MagicPower", "Magic"), UGP_AttributeSet::GetMagicPowerAttribute());
	AddStatSnapshot(FName(TEXT("Armor")), NSLOCTEXT("GPCharacterStatsMenu", "Armor", "Defense"), UGP_AttributeSet::GetArmorAttribute());
	AddStatSnapshot(FName(TEXT("Toughness")), NSLOCTEXT("GPCharacterStatsMenu", "Toughness", "Stagger"), UGP_AttributeSet::GetToughnessAttribute(), UGP_AttributeSet::GetMaxToughnessAttribute());
	AddStatSnapshot(FName(TEXT("AttackSpeed")), NSLOCTEXT("GPCharacterStatsMenu", "AttackSpeed", "Attack Speed"), UGP_AttributeSet::GetAttackSpeedAttribute());
	AddStatSnapshot(FName(TEXT("CriticalChance")), NSLOCTEXT("GPCharacterStatsMenu", "CriticalChance", "Critical Chance"), UGP_AttributeSet::GetCriticalChanceAttribute());
	AddStatSnapshot(FName(TEXT("CritMultiplier")), NSLOCTEXT("GPCharacterStatsMenu", "CritMultiplier", "Crit Multiplier"), UGP_AttributeSet::GetCritMultiplierAttribute());
	AddStatSnapshot(FName(TEXT("DamageIncreaseRate")), NSLOCTEXT("GPCharacterStatsMenu", "DamageIncreaseRate", "Damage Bonus"), UGP_AttributeSet::GetDamageIncreaseRateAttribute());
	AddStatSnapshot(FName(TEXT("Lifesteal")), NSLOCTEXT("GPCharacterStatsMenu", "Lifesteal", "Lifesteal"), UGP_AttributeSet::GetLifestealAttribute());
	AddStatSnapshot(FName(TEXT("MoveSpeed")), NSLOCTEXT("GPCharacterStatsMenu", "MoveSpeed", "Move Speed"), UGP_AttributeSet::GetMoveSpeedAttribute());
	AddStatSnapshot(FName(TEXT("PyrosResistance")), NSLOCTEXT("GPCharacterStatsMenu", "PyrosResistance", "Pyros RES"), UGP_AttributeSet::GetPyrosResistanceAttribute());
	AddStatSnapshot(FName(TEXT("HydroResistance")), NSLOCTEXT("GPCharacterStatsMenu", "HydroResistance", "Hydro RES"), UGP_AttributeSet::GetHydroResistanceAttribute());
	AddStatSnapshot(FName(TEXT("VoltResistance")), NSLOCTEXT("GPCharacterStatsMenu", "VoltResistance", "Volt RES"), UGP_AttributeSet::GetVoltResistanceAttribute());
	AddStatSnapshot(FName(TEXT("AeroResistance")), NSLOCTEXT("GPCharacterStatsMenu", "AeroResistance", "Aero RES"), UGP_AttributeSet::GetAeroResistanceAttribute());
	AddStatSnapshot(FName(TEXT("LuxResistance")), NSLOCTEXT("GPCharacterStatsMenu", "LuxResistance", "Lux RES"), UGP_AttributeSet::GetLuxResistanceAttribute());
	AddStatSnapshot(FName(TEXT("ChaosResistance")), NSLOCTEXT("GPCharacterStatsMenu", "ChaosResistance", "Chaos RES"), UGP_AttributeSet::GetChaosResistanceAttribute());
	AddStatSnapshot(FName(TEXT("BruteResistance")), NSLOCTEXT("GPCharacterStatsMenu", "BruteResistance", "Brute RES"), UGP_AttributeSet::GetBruteResistanceAttribute());

	NotifyAttributeSnapshotsUpdated();
}

bool UGP_CharacterStatsMenuWidget::GetAttributeSnapshotById(FName Id, FGPCharacterStatSnapshot& OutSnapshot) const
{
	for (const FGPCharacterStatSnapshot& Snapshot : AttributeSnapshots)
	{
		if (Snapshot.Id == Id)
		{
			OutSnapshot = Snapshot;
			return true;
		}
	}

	return false;
}

void UGP_CharacterStatsMenuWidget::OnASCInitializedCallback(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	BindToASC(ASC);
}

void UGP_CharacterStatsMenuWidget::HandleBoundAttributeChanged(const FOnAttributeChangeData& Data)
{
	RefreshAttributeSnapshots();
	RefreshAttributeWidgets();
}

void UGP_CharacterStatsMenuWidget::AddStatSnapshot(FName Id, const FText& DisplayName, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute)
{
	if (!BoundAttributeSet.IsValid() || !Attribute.IsValid())
	{
		return;
	}

	FGPCharacterStatSnapshot Snapshot;
	Snapshot.Id = Id;
	Snapshot.DisplayName = DisplayName;
	Snapshot.Attribute = Attribute;
	Snapshot.MaxAttribute = MaxAttribute;
	Snapshot.Value = Attribute.GetNumericValue(BoundAttributeSet.Get());

	if (MaxAttribute.IsValid())
	{
		Snapshot.bHasMaxValue = true;
		Snapshot.MaxValue = MaxAttribute.GetNumericValue(BoundAttributeSet.Get());
		Snapshot.Percent = Snapshot.MaxValue > KINDA_SMALL_NUMBER
			? FMath::Clamp(Snapshot.Value / Snapshot.MaxValue, 0.0f, 1.0f)
			: 0.0f;
	}
	else
	{
		Snapshot.Percent = 0.0f;
	}

	AttributeSnapshots.Add(Snapshot);
}

void UGP_CharacterStatsMenuWidget::AddUniqueAttribute(TArray<FGameplayAttribute>& Attributes, const FGameplayAttribute& Attribute) const
{
	if (Attribute.IsValid() && !Attributes.Contains(Attribute))
	{
		Attributes.Add(Attribute);
	}
}

void UGP_CharacterStatsMenuWidget::AddSnapshotAttributes(TArray<FGameplayAttribute>& Attributes) const
{
	// Subscribe to every stat shown in the Attributes menu so native TextBlocks update in real time.
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetHealthAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetMaxHealthAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetManaAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetMaxManaAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetAttackPowerAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetMagicPowerAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetArmorAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetToughnessAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetMaxToughnessAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetAttackSpeedAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetCriticalChanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetCritMultiplierAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetDamageIncreaseRateAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetLifestealAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetMoveSpeedAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetPyrosResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetHydroResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetVoltResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetAeroResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetLuxResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetChaosResistanceAttribute());
	AddUniqueAttribute(Attributes, UGP_AttributeSet::GetBruteResistanceAttribute());
}

void UGP_CharacterStatsMenuWidget::AddWidgetTreeAttributes(TArray<FGameplayAttribute>& Attributes) const
{
	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this, &Attributes](UWidget* ChildWidget)
	{
		if (const UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(ChildWidget))
		{
			// Embedded attribute bars can declare their own attributes and still share the same GAS subscription path.
			AddUniqueAttribute(Attributes, AttributeWidget->Attribute);
			AddUniqueAttribute(Attributes, AttributeWidget->MaxAttribute);
		}
	});
}

void UGP_CharacterStatsMenuWidget::RefreshAttributeWidgets()
{
	if (!WidgetTree || !BoundAttributeSet.IsValid())
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* ChildWidget)
	{
		if (UGP_AttributeWidget* AttributeWidget = Cast<UGP_AttributeWidget>(ChildWidget))
		{
			TTuple<FGameplayAttribute, FGameplayAttribute> Pair(AttributeWidget->Attribute, AttributeWidget->MaxAttribute);
			if (Pair.Key.IsValid() && Pair.Value.IsValid())
			{
				AttributeWidget->OnAttributeChange(Pair, BoundAttributeSet.Get());
			}
		}
	});
}

void UGP_CharacterStatsMenuWidget::RemoveAttributeBindings()
{
	if (BoundASC.IsValid())
	{
		for (const FGPCharacterStatsMenuAttributeBinding& Binding : AttributeBindings)
		{
			if (Binding.Attribute.IsValid() && Binding.Handle.IsValid())
			{
				// Remove from the exact attribute delegate used during bind to prevent duplicate updates after reopen.
				BoundASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.Handle);
			}
		}
	}

	AttributeBindings.Reset();
}

void UGP_CharacterStatsMenuWidget::NotifyAttributeSnapshotsUpdated()
{
	RefreshStatTextBlocks();

	if (bBroadcastAttributeSnapshotBlueprintEvent)
	{
		BP_OnAttributeSnapshotsUpdated(AttributeSnapshots);
	}
}

void UGP_CharacterStatsMenuWidget::RefreshStatTextBlocks()
{
	if (!bEnableNativeStatTextBinding || !WidgetTree)
	{
		return;
	}

	if (ResolvedTextBindings.Num() == 0 && StatTextBindings.Num() > 0)
	{
		RebuildStatTextBindings();
	}

	for (const FGPResolvedCharacterStatTextBinding& Binding : ResolvedTextBindings)
	{
		UTextBlock* TextBlock = Binding.TextBlock.Get();
		if (!IsValid(TextBlock))
		{
			continue;
		}

		FGPCharacterStatSnapshot Snapshot;
		if (GetAttributeSnapshotById(Binding.StatId, Snapshot))
		{
			TextBlock->SetText(FormatStatText(Snapshot, Binding.Format));
		}
		else
		{
			TextBlock->SetText(FText::FromString(TEXT("--")));
		}
	}
}

void UGP_CharacterStatsMenuWidget::RebuildStatTextBindings()
{
	ResolvedTextBindings.Reset();

	if (!bEnableNativeStatTextBinding || !WidgetTree)
	{
		return;
	}

	for (const FGPCharacterStatTextBinding& Binding : StatTextBindings)
	{
		if (Binding.StatId.IsNone() || Binding.WidgetName.IsNone())
		{
			continue;
		}

		if (UTextBlock* TextBlock = ResolveStatTextBlock(Binding.WidgetName))
		{
			FGPResolvedCharacterStatTextBinding ResolvedBinding;
			ResolvedBinding.StatId = Binding.StatId;
			ResolvedBinding.Format = Binding.Format;
			ResolvedBinding.TextBlock = TextBlock;
			ResolvedTextBindings.Add(ResolvedBinding);
		}
	}
}

UTextBlock* UGP_CharacterStatsMenuWidget::ResolveStatTextBlock(const FName& WidgetName) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return nullptr;
	}

	if (UTextBlock* ExactTextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName)))
	{
		return ExactTextBlock;
	}

	return FindTextBlockByNormalizedName(WidgetName);
}

void UGP_CharacterStatsMenuWidget::RebuildNativeTabBindings()
{
	NativeTabBindings.Reset();
	TabContentSwitcher = ResolveContentSwitcher();

	for (const EGPCharacterMenuTab Tab : GetMenuTabOrder())
	{
		FGPCharacterMenuTabWidgetBinding Binding;
		Binding.Tab = Tab;
		Binding.TabWidget = ResolveWidgetByNames(GetTabWidgetCandidateNames(Tab));
		Binding.ContentWidget = ResolveWidgetByNames(GetContentWidgetCandidateNames(Tab));
		Binding.SelectedIndicatorWidget = ResolveWidgetByNames(GetSelectedIndicatorCandidateNames(Tab));

		BindNativeTabButton(Binding.TabWidget.Get(), Tab);
		NativeTabBindings.Add(Binding);
	}

	ApplyActiveTabState();
}

void UGP_CharacterStatsMenuWidget::BindNativeTabButton(UWidget* TabWidget, EGPCharacterMenuTab Tab)
{
	UButton* Button = Cast<UButton>(TabWidget);
	if (!IsValid(Button))
	{
		return;
	}

	// UButton tabs use direct delegates; text/border tabs are handled by NativeOnMouseButtonDown hit-testing.
	switch (Tab)
	{
	case EGPCharacterMenuTab::Map:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectMapTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectMapTab);
		break;
	case EGPCharacterMenuTab::Journal:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectJournalTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectJournalTab);
		break;
	case EGPCharacterMenuTab::Items:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectItemsTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectItemsTab);
		break;
	case EGPCharacterMenuTab::Attributes:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectAttributesTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectAttributesTab);
		break;
	case EGPCharacterMenuTab::Gear:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectGearTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectGearTab);
		break;
	case EGPCharacterMenuTab::Abilities:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectAbilitiesTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectAbilitiesTab);
		break;
	case EGPCharacterMenuTab::System:
		Button->OnClicked.RemoveDynamic(this, &ThisClass::SelectSystemTab);
		Button->OnClicked.AddDynamic(this, &ThisClass::SelectSystemTab);
		break;
	default:
		break;
	}
}

void UGP_CharacterStatsMenuWidget::ApplyActiveTabState()
{
	ApplyTabVisualState();

	if (!TryActivateSwitcherTab(ActiveTab))
	{
		ApplyNamedContentVisibility();
	}

	if (UTextBlock* TitleTextBlock = Cast<UTextBlock>(ResolveWidgetByNames({ FName(TEXT("TitleText")), FName(TEXT("ActiveTabTitleText")), FName(TEXT("HeaderTitleText")) })))
	{
		TitleTextBlock->SetText(GetMenuTabDisplayName(ActiveTab));
	}
}

void UGP_CharacterStatsMenuWidget::ApplyTabVisualState()
{
	for (const FGPCharacterMenuTabWidgetBinding& Binding : NativeTabBindings)
	{
		const bool bIsActiveTab = Binding.Tab == ActiveTab;
		if (UWidget* TabWidget = Binding.TabWidget.Get())
		{
			if (bEnableNativeTabOpacity)
			{
				TabWidget->SetRenderOpacity(bIsActiveTab ? ActiveTabRenderOpacity : InactiveTabRenderOpacity);
			}
		}

		if (UWidget* IndicatorWidget = Binding.SelectedIndicatorWidget.Get())
		{
			IndicatorWidget->SetVisibility(bIsActiveTab ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

bool UGP_CharacterStatsMenuWidget::TryActivateSwitcherTab(EGPCharacterMenuTab NewTab) const
{
	UWidgetSwitcher* Switcher = TabContentSwitcher.Get();
	if (!IsValid(Switcher) || Switcher->GetChildrenCount() == 0)
	{
		return false;
	}

	for (const FGPCharacterMenuTabWidgetBinding& Binding : NativeTabBindings)
	{
		if (Binding.Tab != NewTab)
		{
			continue;
		}

		if (UWidget* ContentWidget = Binding.ContentWidget.Get())
		{
			const int32 ContentIndex = Switcher->GetChildIndex(ContentWidget);
			if (ContentIndex != INDEX_NONE)
			{
				Switcher->SetActiveWidgetIndex(ContentIndex);
				return true;
			}
		}

		break;
	}

	const int32 TabIndex = GetMenuTabOrder().Find(NewTab);
	if (TabIndex != INDEX_NONE && TabIndex < Switcher->GetChildrenCount())
	{
		Switcher->SetActiveWidgetIndex(TabIndex);
		return true;
	}

	return false;
}

void UGP_CharacterStatsMenuWidget::ApplyNamedContentVisibility() const
{
	bool bHasAnyNamedContent = false;
	for (const FGPCharacterMenuTabWidgetBinding& Binding : NativeTabBindings)
	{
		if (Binding.ContentWidget.IsValid())
		{
			bHasAnyNamedContent = true;
			break;
		}
	}

	if (!bHasAnyNamedContent)
	{
		return;
	}

	for (const FGPCharacterMenuTabWidgetBinding& Binding : NativeTabBindings)
	{
		if (UWidget* ContentWidget = Binding.ContentWidget.Get())
		{
			ContentWidget->SetVisibility(Binding.Tab == ActiveTab ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

bool UGP_CharacterStatsMenuWidget::FindTabAtScreenPosition(const FVector2D& ScreenPosition, EGPCharacterMenuTab& OutTab) const
{
	for (const FGPCharacterMenuTabWidgetBinding& Binding : NativeTabBindings)
	{
		const UWidget* TabWidget = Binding.TabWidget.Get();
		if (!IsValid(TabWidget))
		{
			continue;
		}

		if (TabWidget->GetCachedGeometry().IsUnderLocation(ScreenPosition))
		{
			OutTab = Binding.Tab;
			return true;
		}
	}

	return false;
}

UWidget* UGP_CharacterStatsMenuWidget::ResolveWidgetByNames(const TArray<FName>& WidgetNames) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	for (const FName& WidgetName : WidgetNames)
	{
		if (WidgetName.IsNone())
		{
			continue;
		}

		if (UWidget* ExactWidget = WidgetTree->FindWidget(WidgetName))
		{
			return ExactWidget;
		}

		if (UWidget* NormalizedWidget = FindWidgetByNormalizedName(WidgetName))
		{
			return NormalizedWidget;
		}
	}

	return nullptr;
}

UWidget* UGP_CharacterStatsMenuWidget::FindWidgetByNormalizedName(const FName& WidgetName) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	const FString TargetName = NormalizeWidgetLookupName(WidgetName);
	UWidget* FoundWidget = nullptr;

	WidgetTree->ForEachWidget([&FoundWidget, &TargetName](UWidget* ChildWidget)
	{
		if (FoundWidget || !IsValid(ChildWidget))
		{
			return;
		}

		if (NormalizeWidgetLookupName(ChildWidget->GetFName()).Equals(TargetName, ESearchCase::IgnoreCase))
		{
			FoundWidget = ChildWidget;
		}
	});

	return FoundWidget;
}

UWidgetSwitcher* UGP_CharacterStatsMenuWidget::ResolveContentSwitcher() const
{
	return Cast<UWidgetSwitcher>(ResolveWidgetByNames(ContentSwitcherNames));
}

UTextBlock* UGP_CharacterStatsMenuWidget::FindTextBlockByNormalizedName(const FName& WidgetName) const
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	const FString TargetName = NormalizeWidgetLookupName(WidgetName);
	UTextBlock* FoundTextBlock = nullptr;

	WidgetTree->ForEachWidget([&FoundTextBlock, &TargetName](UWidget* ChildWidget)
	{
		if (FoundTextBlock)
		{
			return;
		}

		UTextBlock* CandidateTextBlock = Cast<UTextBlock>(ChildWidget);
		if (!IsValid(CandidateTextBlock))
		{
			return;
		}

		if (NormalizeWidgetLookupName(CandidateTextBlock->GetFName()).Equals(TargetName, ESearchCase::IgnoreCase))
		{
			FoundTextBlock = CandidateTextBlock;
		}
	});

	return FoundTextBlock;
}

FString UGP_CharacterStatsMenuWidget::NormalizeWidgetLookupName(const FName& Name)
{
	FString NormalizedName = Name.ToString();
	NormalizedName.ReplaceInline(TEXT(" "), TEXT(""));
	NormalizedName.ReplaceInline(TEXT("_"), TEXT(""));
	NormalizedName.ReplaceInline(TEXT("-"), TEXT(""));
	return NormalizedName.ToLower();
}

FText UGP_CharacterStatsMenuWidget::FormatStatText(const FGPCharacterStatSnapshot& Snapshot, EGPCharacterStatTextFormat Format)
{
	FNumberFormattingOptions NumberFormat;
	NumberFormat.SetUseGrouping(false);
	NumberFormat.SetMaximumFractionalDigits(0);
	NumberFormat.SetMinimumFractionalDigits(0);

	const FText CurrentValueText = FText::AsNumber(FMath::RoundToInt(Snapshot.Value), &NumberFormat);
	if (Format == EGPCharacterStatTextFormat::ValueAndMax && Snapshot.bHasMaxValue)
	{
		return FText::Format(
			NSLOCTEXT("GPCharacterStatsMenu", "ValueAndMax", "{0} / {1}"),
			CurrentValueText,
			FText::AsNumber(FMath::RoundToInt(Snapshot.MaxValue), &NumberFormat));
	}

	return CurrentValueText;
}

TArray<EGPCharacterMenuTab> UGP_CharacterStatsMenuWidget::GetMenuTabOrder()
{
	return
	{
		EGPCharacterMenuTab::Map,
		EGPCharacterMenuTab::Journal,
		EGPCharacterMenuTab::Items,
		EGPCharacterMenuTab::Attributes,
		EGPCharacterMenuTab::Gear,
		EGPCharacterMenuTab::Abilities,
		EGPCharacterMenuTab::System
	};
}

TArray<FName> UGP_CharacterStatsMenuWidget::GetTabWidgetCandidateNames(EGPCharacterMenuTab Tab)
{
	switch (Tab)
	{
	case EGPCharacterMenuTab::Map:
		return { FName(TEXT("Button_Map")), FName(TEXT("Button_MapTab")), FName(TEXT("MapTabButton")), FName(TEXT("MapTabWrap")), FName(TEXT("MapTab")) };
	case EGPCharacterMenuTab::Journal:
		return { FName(TEXT("Button_Journal")), FName(TEXT("Button_JournalTab")), FName(TEXT("JournalTabButton")), FName(TEXT("JournalTabWrap")), FName(TEXT("JournalTab")) };
	case EGPCharacterMenuTab::Items:
		return { FName(TEXT("Button_Item")), FName(TEXT("Button_Items")), FName(TEXT("Button_ItemsTab")), FName(TEXT("ItemsTabButton")), FName(TEXT("ItemTabWrap")), FName(TEXT("ItemsTabWrap")), FName(TEXT("ItemsTab")), FName(TEXT("ItemTab")) };
	case EGPCharacterMenuTab::Attributes:
		return { FName(TEXT("Button_Attributes")), FName(TEXT("Button_AttributesTab")), FName(TEXT("AttributesTabButton")), FName(TEXT("AttrributesTabWrap")), FName(TEXT("AttributesTabWrap")), FName(TEXT("AttributesTab")) };
	case EGPCharacterMenuTab::Gear:
		return { FName(TEXT("Button_Gear")), FName(TEXT("Button_GearTab")), FName(TEXT("GearTabButton")), FName(TEXT("GearTabWrap")), FName(TEXT("GearTab")) };
	case EGPCharacterMenuTab::Abilities:
		return { FName(TEXT("Button_Abilities")), FName(TEXT("Button_AbilitiesTab")), FName(TEXT("AbilitiesTabButton")), FName(TEXT("AbilitiesTabWrap")), FName(TEXT("AblitiesTabWrap")), FName(TEXT("AbilitiesTab")), FName(TEXT("AblitiesTab")) };
	case EGPCharacterMenuTab::System:
		return { FName(TEXT("Button_System")), FName(TEXT("Button_SystemTab")), FName(TEXT("SystemTabButton")), FName(TEXT("SystemTabWrap")), FName(TEXT("SystemTab")) };
	default:
		return {};
	}
}

TArray<FName> UGP_CharacterStatsMenuWidget::GetContentWidgetCandidateNames(EGPCharacterMenuTab Tab)
{
	switch (Tab)
	{
	case EGPCharacterMenuTab::Map:
		return { FName(TEXT("MapPanel")), FName(TEXT("MapPage")), FName(TEXT("MapContent")) };
	case EGPCharacterMenuTab::Journal:
		return { FName(TEXT("JournalPanel")), FName(TEXT("JournalPage")), FName(TEXT("JournalContent")) };
	case EGPCharacterMenuTab::Items:
		return { FName(TEXT("ItemsPanel")), FName(TEXT("ItemPanel")), FName(TEXT("InventoryPanel")), FName(TEXT("ItemsPage")), FName(TEXT("ItemPage")), FName(TEXT("InventoryPage")) };
	case EGPCharacterMenuTab::Attributes:
		return { FName(TEXT("AttributesPanel")), FName(TEXT("AttributePanel")), FName(TEXT("AttributesPage")), FName(TEXT("AttributePage")) };
	case EGPCharacterMenuTab::Gear:
		return { FName(TEXT("GearPanel")), FName(TEXT("GearPage")), FName(TEXT("GearContent")) };
	case EGPCharacterMenuTab::Abilities:
		return { FName(TEXT("AbilitiesPanel")), FName(TEXT("AblitiesPanel")), FName(TEXT("AbilitiesPage")), FName(TEXT("AbilityPage")) };
	case EGPCharacterMenuTab::System:
		return { FName(TEXT("SystemPanel")), FName(TEXT("SystemPage")), FName(TEXT("SystemContent")) };
	default:
		return {};
	}
}

TArray<FName> UGP_CharacterStatsMenuWidget::GetSelectedIndicatorCandidateNames(EGPCharacterMenuTab Tab)
{
	switch (Tab)
	{
	case EGPCharacterMenuTab::Map:
		return { FName(TEXT("MapUnderline")), FName(TEXT("MapSelectedIndicator")), FName(TEXT("MapSelectionBar")) };
	case EGPCharacterMenuTab::Journal:
		return { FName(TEXT("JournalUnderline")), FName(TEXT("JournalSelectedIndicator")), FName(TEXT("JournalSelectionBar")) };
	case EGPCharacterMenuTab::Items:
		return { FName(TEXT("ItemsUnderline")), FName(TEXT("ItemUnderline")), FName(TEXT("ItemsSelectedIndicator")), FName(TEXT("ItemSelectionBar")) };
	case EGPCharacterMenuTab::Attributes:
		return { FName(TEXT("AttributesUnderline")), FName(TEXT("AttributeUnderline")), FName(TEXT("AttributesSelectedIndicator")), FName(TEXT("AttributesSelectionBar")) };
	case EGPCharacterMenuTab::Gear:
		return { FName(TEXT("GearUnderline")), FName(TEXT("GearSelectedIndicator")), FName(TEXT("GearSelectionBar")) };
	case EGPCharacterMenuTab::Abilities:
		return { FName(TEXT("AbilitiesUnderline")), FName(TEXT("AblitiesUnderline")), FName(TEXT("AbilitiesSelectedIndicator")), FName(TEXT("AbilitiesSelectionBar")) };
	case EGPCharacterMenuTab::System:
		return { FName(TEXT("SystemUnderline")), FName(TEXT("SystemSelectedIndicator")), FName(TEXT("SystemSelectionBar")) };
	default:
		return {};
	}
}

FText UGP_CharacterStatsMenuWidget::GetMenuTabDisplayName(EGPCharacterMenuTab Tab)
{
	switch (Tab)
	{
	case EGPCharacterMenuTab::Map:
		return NSLOCTEXT("GPCharacterStatsMenu", "MapTab", "Map");
	case EGPCharacterMenuTab::Journal:
		return NSLOCTEXT("GPCharacterStatsMenu", "JournalTab", "Journal");
	case EGPCharacterMenuTab::Items:
		return NSLOCTEXT("GPCharacterStatsMenu", "ItemTab", "Item");
	case EGPCharacterMenuTab::Attributes:
		return NSLOCTEXT("GPCharacterStatsMenu", "AttributesTab", "Attributes");
	case EGPCharacterMenuTab::Gear:
		return NSLOCTEXT("GPCharacterStatsMenu", "GearTab", "Gear");
	case EGPCharacterMenuTab::Abilities:
		return NSLOCTEXT("GPCharacterStatsMenu", "AbilitiesTab", "Abilities");
	case EGPCharacterMenuTab::System:
		return NSLOCTEXT("GPCharacterStatsMenu", "SystemTab", "System");
	default:
		return FText::GetEmpty();
	}
}
