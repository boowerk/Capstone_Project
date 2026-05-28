#include "UI/GP_CharacterStatsMenuWidget.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/GP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/GP_BaseCharacter.h"
#include "Components/Widget.h"
#include "UI/GP_AttributeWidget.h"

void UGP_CharacterStatsMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 에디터에서 만든 WBP를 PlayerController가 초기화하지 못한 경우에도 소유 Pawn 기준으로 자동 바인딩합니다.
	if (!BoundCharacter.IsValid())
	{
		InitializeForCharacter(Cast<AGP_BaseCharacter>(GetOwningPlayerPawn()));
		return;
	}

	SetActiveTab(DefaultTab);
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
		// Pawn 교체나 리스폰 때 이전 캐릭터의 ASC 초기화 이벤트가 남지 않게 정리합니다.
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

		// GAS 값이 바뀌면 텍스트 스냅샷과 GP_AttributeWidget 기반 바를 같이 갱신합니다.
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
	BP_OnAttributeSnapshotsUpdated(AttributeSnapshots);
}

void UGP_CharacterStatsMenuWidget::SetActiveTab(EGPCharacterMenuTab NewTab)
{
	ActiveTab = NewTab;
	BP_OnActiveTabChanged(ActiveTab);
}

void UGP_CharacterStatsMenuWidget::RefreshAttributeSnapshots()
{
	AttributeSnapshots.Reset();

	if (!BoundAttributeSet.IsValid())
	{
		BP_OnAttributeSnapshotsUpdated(AttributeSnapshots);
		return;
	}

	// 현재 GAS AttributeSet을 그대로 메뉴 데이터로 노출해 BP에서는 배치와 스타일만 신경 쓰게 합니다.
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

	BP_OnAttributeSnapshotsUpdated(AttributeSnapshots);
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
	// 메뉴에서 표시하는 기본 능력치들은 모두 델리게이트를 구독해 실시간으로 갱신합니다.
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
			// 에디터에서 HP/Mana 바를 직접 배치한 경우 해당 바의 Attribute도 자동으로 구독합니다.
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
				// 델리게이트를 추가한 정확한 Attribute에서 해제해야 재오픈 시 중복 갱신이 생기지 않습니다.
				BoundASC->GetGameplayAttributeValueChangeDelegate(Binding.Attribute).Remove(Binding.Handle);
			}
		}
	}

	AttributeBindings.Reset();
}
