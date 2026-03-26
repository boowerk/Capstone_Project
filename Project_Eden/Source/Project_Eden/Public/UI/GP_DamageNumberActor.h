#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Items/WeaponItemTypes.h"

#include "GP_DamageNumberActor.generated.h"

class UWidgetComponent;
class UGP_DamageNumberWidget;

UCLASS()
class PROJECT_EDEN_API AGP_DamageNumberActor : public AActor
{
    GENERATED_BODY()

public:
    AGP_DamageNumberActor();

    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    void InitializeDamageNumber(int32 InDamageValue, EWeaponElement InElement);

protected:
    UPROPERTY(VisibleAnywhere, Category = "Damage Number")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, Category = "Damage Number")
    TObjectPtr<UWidgetComponent> WidgetComponent;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Widget")
    TSubclassOf<UGP_DamageNumberWidget> DamageNumberWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
    float LifetimeSeconds = 0.85f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
    float FloatHeight = 120.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
    float SpawnHeightOffset = 110.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number")
    float LateralSpread = 48.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Preview", meta = (ClampMin = "0", UIMin = "0"))
    int32 PreviewDamageValue = 1234;

    UPROPERTY(EditDefaultsOnly, Category = "Damage Number|Preview")
    EWeaponElement PreviewElement = EWeaponElement::Fire;

private:
    void SyncWidgetComponentClass();
    void RefreshWidget();

    UPROPERTY(Transient)
    TObjectPtr<UGP_DamageNumberWidget> DamageWidget;

    UPROPERTY(ReplicatedUsing = OnRep_DamageData)
    int32 DamageValue = 0;

    UPROPERTY(ReplicatedUsing = OnRep_DamageData)
    EWeaponElement DamageElement = EWeaponElement::Fire;

    UFUNCTION()
    void OnRep_DamageData();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    FVector StartLocation = FVector::ZeroVector;
    FVector DriftOffset = FVector::ZeroVector;
    float ElapsedSeconds = 0.0f;
};
