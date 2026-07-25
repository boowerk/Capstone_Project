---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T13:43:15
updated: 2026-07-25T13:43:15
status: active
tags:
  - memoc
  - memoc/worklog
---
# Migrate map and minimap automation tests

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-25T13:43:15

## Summary

- Moved the unchanged Landscape integrity and Minimap capture tests into the Editor-only `Project_EdenTests` module.
- Kept `CrystalSeraphGroggyTests` in the runtime module after its existing `PrismCluster` content contract failed.

## Changed Files

- `Project_Eden/Source/Project_Eden/Private/Tests` (two files removed)
- `Project_Eden/Source/Project_EdenTests/Private/Tests` (two identical files added)

## Verification

- `Project_EdenEditor Win64 Development`: passed.
- Landscape integrity and Minimap capture: passed 2/2.
- `Project_EdenServer Win64 Development`: passed.

## Follow-up

- Resolve the failing Crystal Seraph prism visual-size contract before moving that test.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
