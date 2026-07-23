---
memoc: true
type: worklog
actor: boowerk
created: 2026-07-09T02:52:00+09:00
tags:
  - memoc/worklog
  - region-events
  - gameplay
  - presentation
  - memoc
scope: project-memory
<<<<<<< HEAD
<<<<<<< HEAD
updated: 2026-07-19T07:53:41
=======
updated: 2026-07-21T12:29:05
>>>>>>> origin/main
=======
updated: 2026-07-21T12:29:05
>>>>>>> origin/main
status: active
---
# Add Region Event Examples

## Summary

- Added `UGP_RegionEventData.EventActorClass` so DataAssets can select concrete runtime event actors.
- Added `AGP_RedRiftRegionEventActor`: configured initial wave plus periodic monster waves.
- Added `AGP_RegionEventCrystalNode` and combat-overlap handling so player attacks can destroy non-ASC region objectives.
- Added `AGP_CrystalCorruptionRegionEventActor`: spawns corruption crystals and slows players until all crystals are destroyed.
- Added `AGP_PlayerController::ClientOpenRegionEventAugmentSelect()` and `AGP_ShrineRuinsRegionEventActor`: shrine overlap opens the normal augment picker for the overlapping player.
- Added `AGP_StructureDefenseRegionEventActor`: timed survival event with periodic enemy waves.
- Extended `ProjectEden.Game.RegionEvents.Selection` to cover event actor variants.

## Verified

- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.Game.RegionEvents.Selection` passed.

## Follow-up

- Set each Region Event DataAsset's `EventActorClass` to the desired native example actor.
- Tune `EnemySpawns`, durations, crystal count/radius, slow multiplier, and defense wave intervals in editor.
- PIE-check zone entry, crystal objective destruction, shrine UI on clients, and defense completion timing.
