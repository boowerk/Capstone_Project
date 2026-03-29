#include "Items/WeaponItemTypes.h"

#define LOCTEXT_NAMESPACE "WeaponItemCollection"

UPDA_WeaponItemCollection::UPDA_WeaponItemCollection()
{
    Weapons = {
        {
            TEXT("WP_Common_Fire_Sword"),
            LOCTEXT("CommonFireSwordName", "Ember Longsword"),
            LOCTEXT("CommonFireSwordDesc", "A basic longsword infused with a small fire core."),
            nullptr,
            nullptr,
            80
        },
        {
            TEXT("WP_Common_Water_Spear"),
            LOCTEXT("CommonWaterSpearName", "Wave Spear"),
            LOCTEXT("CommonWaterSpearDesc", "A balanced spear that follows the flow of water."),
            nullptr,
            nullptr,
            82
        },
        {
            TEXT("WP_Common_Lightning_Bow"),
            LOCTEXT("CommonLightningBowName", "Spark Bow"),
            LOCTEXT("CommonLightningBowDesc", "A beginner bow wrapped in a light lightning charge."),
            nullptr,
            nullptr,
            85
        },
        {
            TEXT("WP_Rare_Fire_Axe"),
            LOCTEXT("RareFireAxeName", "Crimson Flame Axe"),
            LOCTEXT("RareFireAxeDesc", "A heavy axe that releases a hotter burst on impact."),
            nullptr,
            nullptr,
            180
        },
        {
            TEXT("WP_Rare_Water_Orb"),
            LOCTEXT("RareWaterOrbName", "Deep Sea Orb"),
            LOCTEXT("RareWaterOrbDesc", "A condensed orb that amplifies water-based magic power."),
            nullptr,
            nullptr,
            190
        },
        {
            TEXT("WP_Rare_Lightning_Daggers"),
            LOCTEXT("RareLightningDaggersName", "Thunder Daggers"),
            LOCTEXT("RareLightningDaggersDesc", "Fast daggers built to stack lightning hits rapidly."),
            nullptr,
            nullptr,
            205
        },
        {
            TEXT("WP_Epic_Fire_Greatsword"),
            LOCTEXT("EpicFireGreatswordName", "Cataclysm Greatsword"),
            LOCTEXT("EpicFireGreatswordDesc", "An epic greatsword with an unstable fire core in its blade."),
            nullptr,
            nullptr,
            420
        },
        {
            TEXT("WP_Epic_Water_Staff"),
            LOCTEXT("EpicWaterStaffName", "Tidal Frost Staff"),
            LOCTEXT("EpicWaterStaffDesc", "A staff tuned for broad water control and freezing bursts."),
            nullptr,
            nullptr,
            430
        },
        {
            TEXT("WP_Epic_Lightning_Hammer"),
            LOCTEXT("EpicLightningHammerName", "Stormbreak Hammer"),
            LOCTEXT("EpicLightningHammerDesc", "A massive hammer that spreads lightning when it lands."),
            nullptr,
            nullptr,
            445
        }
    };
}

bool UPDA_WeaponItemCollection::GetWeaponDataById(FName InItemId, FWeaponItemData& OutWeaponData) const
{
    const FWeaponItemData* FoundWeapon = Weapons.FindByPredicate(
        [&InItemId](const FWeaponItemData& Weapon)
        {
            return Weapon.ItemId == InItemId;
        });

    if (!FoundWeapon)
    {
        return false;
    }

    OutWeaponData = *FoundWeapon;
    return true;
}

#undef LOCTEXT_NAMESPACE
