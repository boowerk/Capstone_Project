---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T05:54:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-23T20:54:04
---
# Session Summary

## Status
- Village Zone boss phase is source-complete and uncommitted. Optional `BossSpawns` starts only after every normal/RegionEvent enemy dies.
- Tagged `EnemySpawnPoint`/`BossSpawnPoint` actors are isolated by Level Instance and constrained to the Zone box.
- Stage Zone/Nav Invoker and Outer-to-Middle travel UI work also remain uncommitted.

## Verified
- Full `Project_EdenEditor Win64 Development` build/link succeeds.

## Pending User
- In each village preset, place ground TargetPoints inside its Zone, tag normal/boss points, and assign one boss class in `Boss Spawns`.
