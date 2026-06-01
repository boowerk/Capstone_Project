---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-30T17:25:42
updated: 2026-05-30T17:25:42
status: active
tags:
  - memoc
  - memoc/worklog
---
# Disable action inertia for distance test

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-05-30T17:25:42

## Summary

- Disabled action inertia calls from Dash, Primary, and DashSlash for target/fallback distance comparison.
- Set `bApplyActionInertiaOnMontageComplete` default to false.

## Changed Files

- `memoc/session-summary.md`
- `Project_Eden/Content/Characters/MaskMan/PDA_MaskMan_AnimationSet.uasset`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_DashSlash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Dash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Primary.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Public/Characters/GP_PlayerCharacter.h`

## Verification

- UHT passed.
- Full UBT blocked by active Live Coding.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
