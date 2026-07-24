---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T14:22:07+09:00
updated: 2026-07-24T14:22:07+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Sans rail idle T-pose fix

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T14:22:07+09:00

## Summary

- Rebuilt the Sans base graph as looping `Sans_Idle_Rail_Loop -> DefaultSlot -> Output`.
- Removed nullable runtime asset-pin overrides that produced Ref Pose outside attacks.
- Added a focused graph, Blueprint assignment, and attack-slot regression test.

## Changed Files

- `Project_Eden/Source/Project_Eden/Private/Utils/GP_AnimationSetupLibrary.cpp`
- `Project_Eden/Source/Project_Eden/Private/Tests/SansBossAnimationTests.cpp`
- Sans AnimBlueprint, animation-set, montages, and boss Blueprint

## Verification

- Rider-equivalent `Project_EdenEditor Win64 Development` build succeeded.
- `GP_ApplySansBossAnimationSetup` completed and saved all Sans assets.
- `ProjectEden.Combat.Sans.AnimationSetup` passed.

## Follow-up

- PIE-check idle, movement, all attacks, and return to idle.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
