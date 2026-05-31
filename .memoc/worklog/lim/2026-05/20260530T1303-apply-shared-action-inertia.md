---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-30T13:03:55
updated: 2026-05-30T13:03:55
status: active
tags:
  - memoc
  - memoc/worklog
---
# Apply shared action inertia

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-05-30T13:03:55

## Summary

- Replaced fallback-only root motion inertia handoff with shared action inertia on montage completion.
- Dash, Primary, and DashSlash now call the same character-level inertia handoff path.

## Changed Files

- `memoc/session-summary.md`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_DashSlash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Dash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Primary.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Public/Characters/GP_PlayerCharacter.h`

## Verification

- `Build.bat Project_EdenEditor Win64 Development ... -WaitMutex` succeeded.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
