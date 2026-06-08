---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-31T14:15:08
updated: 2026-05-31T14:15:08
status: active
tags:
  - memoc
  - memoc/worklog
---
# add skill augment pool data

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-31T14:15:08

## Summary

- Added `UGP_SkillAugmentPoolData` for storing augment DA lists.
- Added `PickRandomAugments(Count)` to return valid non-duplicate random candidates.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/GP_SkillAugmentPoolData.h`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/GP_SkillAugmentPoolData.cpp`

## Verification

- `git diff --check` passed for new pool files.
- Unreal build/PIE not run.

## Follow-up

- Create augment pool DataAsset in editor and wire `PickRandomAugments(3)` into the test controller.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
