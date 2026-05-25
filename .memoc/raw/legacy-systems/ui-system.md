---
memoc: true
type: raw
scope: project-memory
created: 2026-05-21T06:58:20
updated: 2026-05-21T06:58:20
status: active
tags:
  - memoc
  - memoc/system
  - memoc/raw
---
# UI System

## Purpose

Current inventory of UI C++ and Widget Blueprint assets.

## C++ Classes

- `UGP_AttributeWidget`
- `UGP_DebugAttributeRow`
- `UGP_DebugAttributeWidget`
- `UGP_PlayerHUDWidget`
- `UGP_DamageNumberWidget`
- `AGP_DamageNumberActor`
- `UGP_WidgetComponent`

## UI Assets

- `/Game/UI/WBP_DamagNumber`
- `/Game/UI/WBP_EnemyHealthBar`
- `/Game/UI/BP_DamageNumberActor`
- `/Game/UI/Debug/WBP_DebugAttributeRow`
- `/Game/UI/Debug/WBP_DebugAttributeWidget`
- `/Game/UI/HUD/WBP_PlayerHUDWidget`
- `/Game/UI/HUD/WBP_PlayerHealthBar`
- `/Game/UI/HUD/WBP_PlayerManaBar`
- `/Game/UI/HUD/WBP_PlayerStaminaBar`
- `/Game/UI/WidgetComponent/BP_GP_WidgetComponent`

## Observed Links

- `AGP_BaseCharacter` has damage number display flow and a `DamageNumberActorClass`.
- Attribute widgets likely relate to `UGP_AttributeSet` values, but binding details were not opened.

## Verification Limits

- Widget hierarchy, bindings, animations, and HUD creation flow were not opened in Unreal Editor.
