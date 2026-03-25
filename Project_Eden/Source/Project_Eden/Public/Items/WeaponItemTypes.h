#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponItemTypes.generated.h"

class UStaticMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class EWeaponRarity : uint8
{
    Common UMETA(DisplayName = "Common"),
    Rare UMETA(DisplayName = "Rare"),
    Epic UMETA(DisplayName = "Epic")
};

UENUM(BlueprintType)
enum class EWeaponElement : uint8
{
    Fire UMETA(DisplayName = "Fire"),
    Water UMETA(DisplayName = "Water"),
    Lightning UMETA(DisplayName = "Lightning")
};

USTRUCT(BlueprintType)
struct PROJECT_EDEN_API FWeaponItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    FText Name = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (MultiLine = "true"))
    FText Description = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    EWeaponRarity Rarity = EWeaponRarity::Common;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    EWeaponElement Element = EWeaponElement::Fire;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 AttackPower = 10;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 MagicPower = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
    float AttackSpeed = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float CriticalChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 SellPrice = 50;
};

UCLASS(BlueprintType)
class PROJECT_EDEN_API UPDA_WeaponItemCollection : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPDA_WeaponItemCollection();

    UFUNCTION(BlueprintPure, Category = "Weapon")
    bool GetWeaponDataById(FName InItemId, FWeaponItemData& OutWeaponData) const;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TArray<FWeaponItemData> Weapons;
};
