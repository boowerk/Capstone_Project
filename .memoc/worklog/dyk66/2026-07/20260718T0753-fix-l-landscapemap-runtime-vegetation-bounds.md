---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-18T07:53:16
updated: 2026-07-18T07:53:56
status: active
tags:
  - memoc
  - memoc/worklog
---
# Fix L_LandscapeMap runtime vegetation bounds

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-18T07:53:16

## Summary

- Found `L_LandscapeMap` already had a valid serialized Landscape Cache and runtime-partitioned PCG component, but its placed Spawner Box still used the `32cm` template extent.
- Expanded the map instance to `64000,64000,8000`, preserving the shared graph's current `GRID32` grass and `GRID128` default settings.

## Changed Files

- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`
- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/04-handoff.md`
- `.memoc/session-summary.md`

## Verification

- PIE produced 1,911 grass instances at the start and 1,803 around X=400m after moving the pawn; 50 old runtime cells pooled.
- Final readback: Activated, GenerateAtRuntime, Partitioned, Box `64000,64000,8000`, cache `SerializeOnlyAtCook` with 289 entries. No cache or `No surfaces found` error.
- Latest PIE had zero PCG errors; its 64 `Bounds Modifier` warnings came only from separate non-grass branches.

## Follow-up

- Existing non-grass `Bounds Modifier` multiple-input warnings remain and are outside this map-bounds fix.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
