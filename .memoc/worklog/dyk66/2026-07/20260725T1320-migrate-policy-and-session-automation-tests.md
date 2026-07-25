---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T13:20:38
updated: 2026-07-25T13:20:38
status: active
tags:
  - memoc
  - memoc/worklog
---
# Migrate policy and session automation tests

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-25T13:20:38

## Summary

- Moved eight unchanged AI policy, lobby, run-seed, and session automation tests from the runtime module into the Editor-only `Project_EdenTests` module.
- Kept every automation path and existing module dependency unchanged.

## Changed Files

- `Project_Eden/Source/Project_Eden/Private/Tests` (eight files removed)
- `Project_Eden/Source/Project_EdenTests/Private/Tests` (eight identical files added)

## Verification

- `Project_EdenEditor Win64 Development`: passed.
- Eight exact automation filters: passed 8/8.
- `Project_EdenServer Win64 Development`: passed; Server target contains no `Project_EdenTests` reference.

## Follow-up

- Continue migrating tests in small dependency groups.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
