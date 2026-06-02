#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "GP_CharacterStatsMenuWidget.generated.h"

class AGP_BaseCharacter;
class UAbilitySystemComponent;
class UAttributeSet;
class UGP_AttributeSet;
class UTextBlock;

UENUM(BlueprintType)
enum class EGPCharacterMenuTab : uint8
{
	Map,
	Journal,
	Items,
	Attributes,
	Gear,
	Abilities,
	System
};

USTRUCT(BlueprintType)
struct FGPCharacterStatSnapshot
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	FName Id = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	FGameplayAttribute Attribute;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	FGameplayAttribute MaxAttribute;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	float Value = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	float MaxValue = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	float Percent = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "GAS|Menu")
	bool bHasMaxValue = false;
};

UENUM(BlueprintType)
enum class EGPCharacterStatTextFormat : uint8
{
	ValueOnly,
	ValueAndMax
};

USTRUCT(BlueprintType)
struct FGPCharacterStatTextBinding
{
	GENERATED_BODY()

	FGPCharacterStatTextBinding() = default;
	FGPCharacterStatTextBinding(const FName& InStatId, const FName& InWidgetName, EGPCharacterStatTextFormat InFormat = EGPCharacterStatTextFormat::ValueOnly)
		: StatId(InStatId)
		, WidgetName(InWidgetName)
		, Format(InFormat)
	{
	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Menu")
	FName StatId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Menu")
	FName WidgetName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Menu")
	EGPCharacterStatTextFormat Format = EGPCharacterStatTextFormat::ValueOnly;
};

struct FGPCharacterStatsMenuAttributeBinding
{
	FGPCharacterStatsMenuAttributeBinding() = default;
	FGPCharacterStatsMenuAttributeBinding(const FGameplayAttribute& InAttribute, const FDelegateHandle& InHandle)
		: Attribute(InAttribute)
		, Handle(InHandle)
	{
	}

	FGameplayAttribute Attribute;
	FDelegateHandle Handle;
};

struct FGPResolvedCharacterStatTextBinding
{
	FName StatId = NAME_None;
	EGPCharacterStatTextFormat Format = EGPCharacterStatTextFormat::ValueOnly;
	TWeakObjectPtr<UTextBlock> TextBlock;
};

UCLASS(Blueprintable)
class PROJECT_EDEN_API UGP_CharacterStatsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UGP_CharacterStatsMenuWidget(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu")
	void InitializeForCharacter(AGP_BaseCharacter* InCharacter);

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu")
	void BindToASC(UAbilitySystemComponent* InASC);

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu")
	void ClearASC();

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu")
	void SetActiveTab(EGPCharacterMenuTab NewTab);

	UFUNCTION(BlueprintPure, Category = "GAS|Menu")
	EGPCharacterMenuTab GetActiveTab() const { return ActiveTab; }

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu")
	void RefreshAttributeSnapshots();

	UFUNCTION(BlueprintPure, Category = "GAS|Menu")
	TArray<FGPCharacterStatSnapshot> GetAttributeSnapshots() const { return AttributeSnapshots; }

	UFUNCTION(BlueprintPure, Category = "GAS|Menu")
	bool GetAttributeSnapshotById(FName Id, FGPCharacterStatSnapshot& OutSnapshot) const;

	UFUNCTION(BlueprintCallable, Category = "GAS|Menu|Text")
	void RefreshStatTextBlocks();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|Menu")
	void BP_OnMenuBound(AGP_BaseCharacter* Character);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|Menu")
	void BP_OnActiveTabChanged(EGPCharacterMenuTab NewTab);

	UFUNCTION(BlueprintImplementableEvent, Category = "GAS|Menu")
	void BP_OnAttributeSnapshotsUpdated(const TArray<FGPCharacterStatSnapshot>& Snapshots);

private:
	// Default to the Attributes page because Tab is meant to open the character stat screen.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Menu", meta = (AllowPrivateAccess = "true"))
	EGPCharacterMenuTab DefaultTab = EGPCharacterMenuTab::Attributes;

	// Keeps stat TextBlocks driven from C++ so the menu does not need a Blueprint graph for value binding.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Menu|Text", meta = (AllowPrivateAccess = "true"))
	bool bEnableNativeStatTextBinding = true;

	// Disabled by default because native text binding is now the primary path; enable only for custom BP reactions.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Menu|Text", meta = (AllowPrivateAccess = "true"))
	bool bBroadcastAttributeSnapshotBlueprintEvent = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Menu|Text", meta = (AllowPrivateAccess = "true"))
	TArray<FGPCharacterStatTextBinding> StatTextBindings;

	TWeakObjectPtr<AGP_BaseCharacter> BoundCharacter;
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;
	TWeakObjectPtr<UGP_AttributeSet> BoundAttributeSet;
	TArray<FGPCharacterStatsMenuAttributeBinding> AttributeBindings;
	TArray<FGPResolvedCharacterStatTextBinding> ResolvedTextBindings;
	TArray<FGPCharacterStatSnapshot> AttributeSnapshots;
	EGPCharacterMenuTab ActiveTab = EGPCharacterMenuTab::Attributes;

	UFUNCTION()
	void OnASCInitializedCallback(UAbilitySystemComponent* ASC, UAttributeSet* AS);

	void HandleBoundAttributeChanged(const FOnAttributeChangeData& Data);
	void AddStatSnapshot(FName Id, const FText& DisplayName, const FGameplayAttribute& Attribute, const FGameplayAttribute& MaxAttribute = FGameplayAttribute());
	void AddUniqueAttribute(TArray<FGameplayAttribute>& Attributes, const FGameplayAttribute& Attribute) const;
	void AddSnapshotAttributes(TArray<FGameplayAttribute>& Attributes) const;
	void AddWidgetTreeAttributes(TArray<FGameplayAttribute>& Attributes) const;
	void RefreshAttributeWidgets();
	void RemoveAttributeBindings();
	void NotifyAttributeSnapshotsUpdated();
	void RebuildStatTextBindings();
	UTextBlock* ResolveStatTextBlock(const FName& WidgetName) const;
	UTextBlock* FindTextBlockByNormalizedName(const FName& WidgetName) const;
	static FString NormalizeWidgetLookupName(const FName& Name);
	static FText FormatStatText(const FGPCharacterStatSnapshot& Snapshot, EGPCharacterStatTextFormat Format);
};
