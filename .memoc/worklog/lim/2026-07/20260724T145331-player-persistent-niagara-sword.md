---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T14:53:31+09:00
updated: 2026-07-24T14:53:31+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Persistent player Niagara sword visual

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T14:53:31+09:00

## Summary

- Added an always-visible player sword from `NS_Big_Sword`'s actual mesh renderer.
- Reused `SM_7` and its runtime `MI_Ice_Inst_4` override without keeping the one-shot Niagara smoke/spark emitters active.
- Attached to deforming `hand_r`; disabled inherited scale so Paladin's `0.14` visual correction does not shrink the sword.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Characters/GP_PlayerCharacter.h`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Private/Tests/PlayerWeaponVisualTests.cpp`

## Verification

- Rider-equivalent `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Player.WeaponVisual.NiagaraSource` passed.

## Follow-up

- PIE-check P1/P2/P3 hand alignment and static-component material appearance.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
