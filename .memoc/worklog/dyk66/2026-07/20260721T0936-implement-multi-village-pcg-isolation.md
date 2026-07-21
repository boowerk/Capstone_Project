---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-21T09:36:22
updated: 2026-07-21T19:30:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Implement multi-village PCG isolation

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-21T09:36:22

## Summary

- Added multi-slot village level streaming with slot-unique Road/District tags and per-component PCG parameter overrides.
- Added all-loaded/all-shown barriers, PCG completion/cancellation tracking, timeout rollback, and graph-contract tests.
- Serialized city PCG generation per selected slot and added managed-resource/instance/location/bounds diagnostics after a parallel run displayed only one village.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/Game/WorldLayout/GP_VillageLayoutDirector.h`
- `Project_Eden/Source/Project_Eden/Private/Game/WorldLayout/GP_VillageLayoutDirector.cpp`
- `Project_Eden/Source/Project_Eden/Private/Tests/VillageSelectionTests.cpp`
- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`

## Verification

- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.WorldLayout.VillageSelection` and `ProjectEden.Game.RunSeed.Flow` passed.
- Saved-map audit confirmed 3 slots, no fixed Level Instance, and current Required/PickCount=2 rule.
- Headless `L_LandscapeMap` runtime first generated Village_A/B, then reproduced original RunSeed `815718662`: Village_B/C used distinct tags/transforms and generated 314/274 PCG-managed ISM instances with distinct bounds, sequentially.

## Follow-up

- Optionally visually confirm both isolated villages in PIE; runtime metrics already prove two non-empty outputs.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
