---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T14:15:00
updated: 2026-07-25T14:15:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Migrate Sans AnimGraph automation test

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-25T14:15:00

## Summary

- Added `AnimGraph` and `BlueprintGraph` to the Editor-only test module and moved the unchanged Sans animation graph contract test into it.
- Verified the MinimalAPI graph-node calls link across the module boundary.

## Changed Files

- `Project_Eden/Source/Project_EdenTests/Project_EdenTests.Build.cs`
- Sans boss animation test source location

## Verification

- `Project_EdenEditor Win64 Development`: passed.
- `ProjectEden.Combat.Sans.AnimationSetup`: passed.
- `Project_EdenServer Win64 Development`: passed.

## Follow-up

- Audit and resolve the remaining GameplayTag export boundary before moving its test group.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
