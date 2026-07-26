---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-26T18:16:00+09:00
updated: 2026-07-26T18:16:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Grounded enemy spawns

actor: dyk66
actor_source: OS user
branch: fix/grounded-enemy-spawns
status: done
created: 2026-07-26T18:16:00+09:00

## Summary

- Traced intermittent house-roof spawning to broad vertical NavMesh projection after random XY scatter.
- Anchored regular and marker spawns to tight authored ground points and selected scatter only with reachable-nav queries.
- Added separate rise guards for nav candidates and collision-adjusted capsule feet.
- Preserved boss Level Instance offset recovery while requiring broad candidates to remain connected to a trusted ground anchor.
- Made unsafe regular and all-or-nothing boss placement retry without prematurely resolving zone progression.

## Changed Files

- `Source/Project_Eden/Public/Game/GP_EnemySpawnVolume.h`
- `Source/Project_Eden/Private/Game/GP_EnemySpawnVolume.cpp`
- `Source/Project_Eden/Private/Game/GP_GameMode_ZoneCombat.cpp`
- `Source/Project_EdenTests/Private/Tests/EnemySpawnPlacementTests.cpp`

## Verification

- `Project_EdenEditor Win64 Development` succeeded.
- `ProjectEden.Game.EnemySpawnPlacement.GroundPolicy` succeeded.
- `ProjectEden.Game.ZoneProgression.Contracts` succeeded.
- Independent final patch audit found no mandatory logic or Unreal runtime correction.

## Follow-up

- Run a live/PIE spawn sweep in all four village presets, including boss phase.
- If an intentionally elevated walkable combat platform is rejected, tune the per-zone rise allowance rather than restoring broad unconnected projection.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
