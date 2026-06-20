---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-20T12:29:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-20T12:29:00+09:00
Replace, do not append. Keep <800B.

## Status
- `/Game/Maps/MainMap/L_GameMap` has 9 jittered `BP_RegionSeed`, 3 `BP_CityAnchor`, 1 curved `BP_CityRoadSpline`, and `RegionStateManager`.
- Layout used fixed random seed `1337`; seed BaseType uses `CITY_CANDIDATE` for region indices `0,7,2`, `NATURE` for others.

## Verified
- Unreal Python verified actor labels, positions, city anchor `TargetRegionIndex`, nearest target seed, road spline points, `RegionCount=9`, `StateRT=RT_RegionState_9x1`, `RegionStateCount=4`, and saved `L_GameMap`.

## Resume
- RegionIdTexture left unchanged; bake remains pending.
