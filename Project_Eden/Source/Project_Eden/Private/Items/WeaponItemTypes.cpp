#include "Items/WeaponItemTypes.h"

#define LOCTEXT_NAMESPACE "WeaponItemCollection"

UPDA_WeaponItemCollection::UPDA_WeaponItemCollection()
{
    Weapons = {
        {
            TEXT("WP_Common_Fire_Sword"),
            LOCTEXT("CommonFireSwordName", "Ember Longsword"),
            LOCTEXT("CommonFireSwordDesc", "A basic longsword infused with a small fire core."),
            EWeaponRarity::Common,
            EWeaponElement::Fire,
            nullptr,
            nullptr,
            14,
            4,
            1.00f,
            0.05f,
            80
        },
        {
            TEXT("WP_Common_Water_Spear"),
            LOCTEXT("CommonWaterSpearName", "Wave Spear"),
            LOCTEXT("CommonWaterSpearDesc", "A balanced spear that follows the flow of water."),
            EWeaponRarity::Common,
            EWeaponElement::Water,
            nullptr,
            nullptr,
            13,
            5,
            1.05f,
            0.04f,
            82
        },
        {
            TEXT("WP_Common_Lightning_Bow"),
            LOCTEXT("CommonLightningBowName", "Spark Bow"),
            LOCTEXT("CommonLightningBowDesc", "A beginner bow wrapped in a light lightning charge."),
            EWeaponRarity::Common,
            EWeaponElement::Lightning,
            nullptr,
            nullptr,
            12,
            6,
            1.10f,
            0.06f,
            85
        },
        {
            TEXT("WP_Rare_Fire_Axe"),
            LOCTEXT("RareFireAxeName", "Crimson Flame Axe"),
            LOCTEXT("RareFireAxeDesc", "A heavy axe that releases a hotter burst on impact."),
            EWeaponRarity::Rare,
            EWeaponElement::Fire,
            nullptr,
            nullptr,
            22,
            8,
            0.92f,
            0.10f,
            180
        },
        {
            TEXT("WP_Rare_Water_Orb"),
            LOCTEXT("RareWaterOrbName", "Deep Sea Orb"),
            LOCTEXT("RareWaterOrbDesc", "A condensed orb that amplifies water-based magic power."),
            EWeaponRarity::Rare,
            EWeaponElement::Water,
            nullptr,
            nullptr,
            16,
            18,
            1.00f,
            0.08f,
            190
        },
        {
            TEXT("WP_Rare_Lightning_Daggers"),
            LOCTEXT("RareLightningDaggersName", "Thunder Daggers"),
            LOCTEXT("RareLightningDaggersDesc", "Fast daggers built to stack lightning hits rapidly."),
            EWeaponRarity::Rare,
            EWeaponElement::Lightning,
            nullptr,
            nullptr,
            18,
            11,
            1.22f,
            0.12f,
            205
        },
        {
            TEXT("WP_Epic_Fire_Greatsword"),
            LOCTEXT("EpicFireGreatswordName", "Cataclysm Greatsword"),
            LOCTEXT("EpicFireGreatswordDesc", "An epic greatsword with an unstable fire core in its blade."),
            EWeaponRarity::Epic,
            EWeaponElement::Fire,
            nullptr,
            nullptr,
            34,
            15,
            0.90f,
            0.18f,
            420
        },
        {
            TEXT("WP_Epic_Water_Staff"),
            LOCTEXT("EpicWaterStaffName", "Tidal Frost Staff"),
            LOCTEXT("EpicWaterStaffDesc", "A staff tuned for broad water control and freezing bursts."),
            EWeaponRarity::Epic,
            EWeaponElement::Water,
            nullptr,
            nullptr,
            22,
            30,
            0.98f,
            0.15f,
            430
        },
        {
            TEXT("WP_Epic_Lightning_Hammer"),
            LOCTEXT("EpicLightningHammerName", "Stormbreak Hammer"),
            LOCTEXT("EpicLightningHammerDesc", "A massive hammer that spreads lightning when it lands."),
            EWeaponRarity::Epic,
            EWeaponElement::Lightning,
            nullptr,
            nullptr,
            32,
            18,
            0.95f,
            0.20f,
            445
        }
    };
}

#undef LOCTEXT_NAMESPACE
