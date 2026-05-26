---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-22T17:57:30
updated: 2026-05-22T17:57:30
status: active
tags:
  - memoc
  - memoc/worklog
---
# Add thrown burst projectile path

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-22T17:57:30

## Summary

- Added `AGP_AreaProjectile` for thrown impact explosions.
- Added `UGP_Skill_ThrownBurst` and `GPTags.Cooldown.Skill.ThrownBurst`.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Actors/GP_AreaProjectile.h`
- `Project_Eden/Source/Project_Eden/Private/Actors/GP_AreaProjectile.cpp`
- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/Player/Character1/GP_Skill_ThrownBurst.h`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_ThrownBurst.cpp`
- `Project_Eden/Source/Project_Eden/Public/GameplayTags/GP_Tags.h`
- `Project_Eden/Source/Project_Eden/Private/GameplayTags/GP_Tags.cpp`

## Verification

- Build not run per user request; user will verify with their own bat/editor flow.

## Follow-up

- Create BP area projectile, BP GA, DA skill, and add it to the test skill set.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
