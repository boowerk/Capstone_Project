---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T13:39:22+09:00
updated: 2026-07-23T13:39:22+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Sync village preview seed in PIE

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-23T13:39:22+09:00

## Summary

- Added an enabled-by-default Director switch that uses `PreviewSeed` for village selection in PIE.
- Kept standalone and packaged runtime selection on the authoritative GameState RunSeed.
- Logged the PreviewSeed and GameState RunSeed when the PIE override is active.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Game/WorldLayout/GP_VillageLayoutDirector.h`
- `Project_Eden/Source/Project_Eden/Private/Game/WorldLayout/GP_VillageLayoutDirector.cpp`
- `Project_Eden/Source/Project_Eden/Private/Tests/VillageSelectionTests.cpp`

## Verification

- Full `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.WorldLayout.VillageSelection` passed.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
