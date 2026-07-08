---
memoc: true
type: worklog
actor: boowerk
created: 2026-07-09T02:08:00+09:00
tags:
  - memoc/worklog
  - region-events
  - pcg
  - gameplay
---
# Add Region Event System

## Summary

- Added `UGP_RegionEventData` so designers can author event type, trigger, weighted selection, duration, optional region-state writes, icon metadata, and enemy waves.
- Added `GPRegionEventSelectionPolicy` and `ProjectEden.Game.RegionEvents.Selection` automation for deterministic weighted event selection.
- Added replicated `AGP_RegionEventActor` with BP presentation hooks and optional enemy spawning.
- Added `AGP_RegionEventDirector` to choose weighted events for a zone and spawn event actors near `AGP_EnemySpawnVolume`.
- Integrated `AGP_GameMode` so zone-start events roll by default and event-spawned enemies count toward the active zone clear.

## Verified

- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.RegionEvents.Selection` passed under `UnrealEditor-Cmd -NullRHI`.

## Follow-up

- Create editor event data assets and a BP child of `AGP_RegionEventActor` for VFX/decal/UI presentation.
- Place/configure `AGP_RegionEventDirector` in the main map, add event assets to `EventPool`, and PIE-check zone entry/clear behavior.
