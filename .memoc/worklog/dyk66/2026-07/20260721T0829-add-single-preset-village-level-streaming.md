---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-21T08:29:53
updated: 2026-07-21T17:32:45+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# add single-preset village level streaming

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-21T08:29:53

## Summary

- Streamed `L_Village_00` at the deterministically selected village slot.
- Gated PCG until level load/show and seeded it from RunSeed plus SlotId.
- Kept the optional Runtime Data Layer path disabled by default.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Game/WorldLayout/GP_VillageLayoutDirector.h`
- `Project_Eden/Source/Project_Eden/Private/Game/WorldLayout/GP_VillageLayoutDirector.cpp`
- `Project_Eden/Source/Project_Eden/Public/Game/WorldLayout/GP_VillageSlot.h`
- `Project_Eden/Source/Project_Eden/Private/Tests/VillageSelectionTests.cpp`
- `Project_Eden/Content/WorldLayout/L_Village_00.umap`
- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`

## Verification

- Full `Project_EdenEditor` build succeeded.
- `ProjectEden.Game.RunSeed.Flow` and `ProjectEden.Game.WorldLayout.VillageSelection` passed.
- PIE selected `Village_B`, configured/generated one PCG component, and the user visually confirmed placement.

## Follow-up

- Add per-instance PCG discovery before enabling multiple villages.
- Add multiplayer client synchronization and packaging cook registration.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
