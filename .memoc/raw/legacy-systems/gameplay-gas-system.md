---
memoc: true
type: raw
scope: project-memory
created: 2026-05-21T06:58:20
updated: 2026-05-23T03:10:00+09:00
status: active
tags:
  - memoc
  - memoc/system
  - memoc/raw
---
# Gameplay and GAS System

## Purpose

Current inventory of character, ability, attribute, and gameplay assets.

## C++ Character Classes

- `AGP_BaseCharacter`: abstract `ACharacter` implementing `IAbilitySystemInterface`; owns startup abilities, initialize attributes effect, animation set update, ASC initialized event, and damage number display flow.
- `AGP_PlayerCharacter`: player character; owns camera components, sprint/dash helpers, lock-on fields, default weapon collection/id, and runtime skill equip APIs.
- `AGP_EnemyCharacter`: enemy character; owns behavior anchor/ranges, perception tuning, behavior tree/blackboard overrides, enemy archetype data, boss flag/display name, and ASC/AttributeSet fields.

## C++ GAS Classes

- `UGP_AbilitySystemComponent`
- `UGP_AttributeSet`
- `UGP_DamageExecCalculation`
- `UGP_GameplayAbility`
- `UGP_SkillBase`
- `UGP_SkillData`
- Player abilities: `UGP_Primary`, `UGP_Dash`, `UGP_Sprint`, `UGP_Ultimate`, `UGP_Skill_WaterPuddle`
- Enemy abilities: `UGP_EnemyAttack`, `UGP_HitReact`

## Skill Production Rules

Use `UGP_SkillData` as the per-skill data source. Runtime equip stores the
data asset on `FGameplayAbilitySpec::SourceObject`; abilities/projectiles read
that object and pass it to `ApplyGameplayEffectToActors` so shared damage GE
specs receive SetByCaller damage values.

Damage effect ownership:

- GA-owned damage: if the ability directly finds targets and applies damage,
  set `DamageEffectClass` on the GA BP. Example: `GroundBurst`.
- Actor/projectile-owned damage: if the GA only spawns an actor/projectile and
  the spawned object later applies damage, set `DamageEffectClass` on that BP.
  Examples: `NetTestProjectile`, `SplitShot`, `ThrownBurst`.
- Prefer `GE_Damage_Generic` for common formulas. Create per-skill damage GEs
  only when the formula, status behavior, or periodic behavior differs.

Cooldown ownership:

- Normal skills use `UGP_SkillBase` generic cooldown, not the built-in
  `Cooldown Gameplay Effect Class` field.
- GA BP: leave built-in cooldown class empty; set
  `GenericCooldownEffectClass = GE_Cooldown_Generic`.
- SkillData: set `CooldownPolicy = Generic`, a unique
  `GPTags.Cooldown.Skill.*` tag, and duration.
- Special/manual skills can use `CooldownPolicy = Custom`; `WaterPuddle` is the
  current example.

Area effect helpers:

- `SphereOverlapActorsAtLocation` returns pawns inside a sphere at an explicit
  world location.
- `ApplyAreaGameplayEffectAtLocation` overlaps at a location and applies a GE.
- If HitReact or extra events are needed, call
  `SphereOverlapActorsAtLocation`, then `ApplyGameplayEffectToActors`, then
  `SendGameplayEventToActors`.

Current skill patterns:

- `GroundBurst`: GA chooses controller-yaw forward ground location, overlaps at
  that location, applies SkillData damage, sends Enemy HitReact, and can spawn
  a visual actor.
- `ThrownBurst`: GA spawns `AGP_AreaProjectile`; projectile explodes on
  hit/overlap, spawns impact visual by multicast, overlaps at impact location,
  applies SkillData damage, and sends Enemy HitReact.

BP setup checklist:

- GA BP from native skill class.
- Set `GenericCooldownEffectClass = GE_Cooldown_Generic`.
- Do not set built-in `Cooldown Gameplay Effect Class` for generic cooldowns.
- Set `DamageEffectClass` on the actual damage owner: GA or projectile/actor.
- Projectile visual mesh should use no collision when C++ collision component
  owns hit detection.
- DA skill sets name/description/icon, `AbilityClass`, supported slots, damage
  values, `CooldownPolicy`, cooldown tag, and duration.

Balance seed values:

- `GroundBurst`: BaseDamage 25, ToughnessDamage 10, AtkCoef 0.5,
  Cooldown 5.0.
- `ThrownBurst`: BaseDamage 35, ToughnessDamage 15, AtkCoef 0.6,
  Cooldown 6.0.
- Heuristic: instant/self-centered skills get lower damage and shorter
  cooldown; aimed ground skills are mid; thrown/impact skills can be stronger
  with longer cooldown because travel/aim adds risk.

Next recommended skill after cleanup: `PulseBurst`, a player-centered instant
AoE. It validates the third position-source pattern after ground-targeted and
projectile-impact explosions.

## AttributeSet Fields Observed

- Meta: `Damage`, `ToughnessDamage`
- Core: `Health`, `MaxHealth`, `Mana`, `MaxMana`
- Weapon/offense: `AttackPower`, `MagicPower`, `AttackSpeed`, `CriticalChance`, `CritMultiplier`, `DamageIncreaseRate`
- Defensive/resistance: `Armor`, `PyrosResistance`, `HydroResistance`, `VoltResistance`, `AeroResistance`, `LuxResistance`, `ChaosResistance`, `BruteResistance`
- Toughness: `Toughness`, `MaxToughness`, `ToughnessRecoveryRate`
- Utility: `MoveSpeed`
- Events: `OnAttributesInitialized`, `OnDamageTaken`

## Blueprint and Data Assets

Game mode and player:

- `/Game/GAS_Pattern/Game/BP_ProjectEden_Gamemode`
- `/Game/GAS_Pattern/Player/BP_GP_PlayerController`
- `/Game/GAS_Pattern/Player/BP_GP_PlayerState`
- `/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter`

Abilities:

- `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Primary`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Dash`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Sprint`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/GA_Targeting`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/PlayerAbilities/Character1/GA_Skill_WaterPuddle`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/EnemyAbilities/GA_EnemyAttack`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/EnemyAbilities/GA_HitReact`
- `/Game/GAS_Pattern/AbilitySystem/Abilities/EnemyAbilities/GA_AnimNotify_SendGameplayEvent`

Gameplay effects:

- `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_InitializeAttributes`
- `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_PrimaryDamage`
- `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_Cooldown_WaterPuddle`
- `/Game/GAS_Pattern/AbilitySystem/GameplayEffects/GE_PuddleDebuff`

Items:

- `/Game/Items/Weapons/PDA_WeaponCollection_Main`

## Verification Limits

- Blueprint class parent settings, ability tags, gameplay effect modifiers, input bindings, and runtime behavior were not opened in Unreal Editor.
- C++ build was not run.
