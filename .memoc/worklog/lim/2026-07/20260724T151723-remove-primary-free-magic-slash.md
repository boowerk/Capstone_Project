---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T15:17:23+09:00
updated: 2026-07-24T15:17:23+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Remove primary Free Magic Slash notifies

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T15:17:23+09:00

## Summary

- Traced the purple basic-attack slash past the disabled SkillData burst to direct montage notifies.
- Removed one `AnimNotify_PlayNiagaraEffect -> NS_Free_Magic_Slash` from each source primary montage A-D.
- Preserved timed `NS_ArrowTrail_Magic` and all gameplay event notifies.
- Added regression coverage that reloads all four montages and verifies the surgical removal.

## Changed Files

- `AM_UEFN_Sword_Light_A.uasset`
- `AM_UEFN_Sword_Light_B.uasset`
- `AM_UEFN_Sword_Light_C.uasset`
- `AM_UEFN_Sword_Light_D.uasset`
- `Project_Eden/Source/Project_Eden/Private/Tests/PlayerWeaponVisualTests.cpp`

## Verification

- Rider-equivalent `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Player.WeaponVisual` passed 2/2 in a fresh editor process.

## Follow-up

- PIE-check all four primary combo swings; Free Magic Slash must be absent.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
