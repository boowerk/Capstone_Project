---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-22T13:51:10
updated: 2026-05-22T13:51:10
status: active
tags:
  - memoc
  - memoc/worklog
---
# Add shared area effect helpers

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-22T13:51:10

## Summary

- Added explicit-location sphere overlap and area GE apply helpers to `UGP_BlueprintLibrary`.
- Refactored `SphereMeleeHitBoxOverlap` to calculate the forward location, then reuse the shared overlap helper.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Utils/GP_BlueprintLibrary.h`
- `Project_Eden/Source/Project_Eden/Private/Utils/GP_BlueprintLibrary.cpp`

## Verification

- Built `Project_EdenEditor Win64 Development` with UE_5.7 `Build.bat`; result succeeded.

## Follow-up

- Create `UGP_Skill_GroundBurst` using controller yaw, ground trace, and `ApplyAreaGameplayEffectAtLocation`.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
