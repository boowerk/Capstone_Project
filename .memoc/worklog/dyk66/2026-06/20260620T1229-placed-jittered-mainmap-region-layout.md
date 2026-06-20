---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-20T12:29:21
updated: 2026-06-20T12:29:21
status: active
tags:
  - memoc
  - memoc/worklog
---
# Placed jittered MainMap region layout

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-06-20T12:29:21

## Summary

- Regenerated `/Game/Maps/MainMap/L_GameMap` region seeds, city anchors, and road spline with fixed random seed `1337`.
- Set city-candidate seed indices to `0,7,2`; anchors target those same region indices and remain inside their target Voronoi cells.
- Updated `BP_RegionStateManager` defaults so the placed manager reads `RegionCount=9`, `StateRT=RT_RegionState_9x1`, and `RegionStateCount=4`.

## Changed Files

- `Project_Eden/Content/Maps/MainMap/L_GameMap.umap`
- `Project_Eden/Content/RegionSystem/Blueprints/BP_RegionStateManager.uasset`

## Verification

- Unreal Python readback verified final seed/anchor locations, anchor nearest-seed targets, 5 spline points, state manager fields, and `save_current_level=True`.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
