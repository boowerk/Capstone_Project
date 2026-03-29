#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WeaponItemTypes.generated.h"

class UStaticMesh;
class UTexture2D;
class UGameplayEffect;

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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Info")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Info")
    FText Name = FText::GetEmpty();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Info", meta = (MultiLine = "true"))
    FText Description = FText::GetEmpty();
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
    TSoftObjectPtr<UStaticMesh> Mesh;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0"))
    int32 SellPrice = 50;

    // ASC에 적용될 유지형 GE 클래스
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|GAS")
    TSubclassOf<UGameplayEffect> EquipEffectClass;
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
