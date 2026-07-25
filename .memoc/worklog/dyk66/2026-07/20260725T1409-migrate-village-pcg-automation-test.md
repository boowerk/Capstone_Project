---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T14:09:23
updated: 2026-07-25T14:09:23
status: active
tags:
  - memoc
  - memoc/worklog
---
# Migrate Village PCG automation test

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-25T14:09:23

## Summary

- Added `PCG` to the Editor-only test module and moved the unchanged Village selection/PCG asset contract test into it.

## Changed Files

- `Project_Eden/Source/Project_EdenTests/Project_EdenTests.Build.cs`
- Village selection test source location

## Verification

- `Project_EdenEditor Win64 Development`: passed.
- `ProjectEden.Game.WorldLayout.VillageSelection`: passed.
- `Project_EdenServer Win64 Development`: passed.

## Follow-up

- Continue with the isolated Sans AnimGraph test.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
