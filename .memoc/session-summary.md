---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-20T22:54:23+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-20T23:45:00+09:00
Replace, do not append. Keep <800B.

## Status
- C++ region startup timing patched: `AGP_GameMode::BeginPlay` now schedules `InitializeRegionStates()` on next tick so placed `BP_RegionStateManager` can bind to `AGP_GameState::OnRegionStateChanged` before the initial all-dead reset broadcast.
- Region state mapping confirmed from runtime test: `0 = healthy/alive`; C++ defaults changed to `DeadRegionState=3`, `AliveRegionState=0`.
- `/Game/Maps/MainMap/L_GameMap` now has gameplay test setup: `NavMeshBounds_MainMap`, `RecastNavMesh-Default`, `SpawnZone_0..2`, and one `PlayerStart_City1`.
- Spawn zones use `AGP_EnemySpawnVolume`, 4000/4000/1000 extents, melee basic enemy x3, orders 0/1/2, revive regions `[1,3]`, `[4,6]`, `[5,8]`.

## Verified
- Unreal Python verified zone/player/nav actor properties, `RegionCount=9`, `RegionIdTexture=T_GameMap_RegionID`, `StateRT=RT_RegionState_9x1`; `manage_navigation rebuild_navigation` reported navmesh present and the map saved.
- `L_GameMap` uses `BP_ProjectEden_Gamemode`, parent `GP_GameMode`; CDO has `RegionCount=9`, dead `0`, alive `3`, `GameStateClass=GP_GameState`.

## Resume
- Rebuild/restart editor or Live Coding compile before retesting; ensure `BP_ProjectEden_Gamemode` overrides match dead `3`, alive `0`.
