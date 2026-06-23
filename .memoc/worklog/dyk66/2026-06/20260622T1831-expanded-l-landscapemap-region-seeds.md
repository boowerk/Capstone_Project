---
memoc: true
type: worklog
scope: project-memory
created: 2026-06-22T18:31:50
updated: 2026-06-22T18:31:50
status: active
tags:
  - memoc
  - memoc/worklog
---
# Expanded L_LandscapeMap region seeds

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-06-22T18:31:50

## Summary

- Expanded `/Game/Maps/MainMap/L_LandscapeMap` from 9 to 15 `BP_RegionSeed` actors with a loose 5x3 layout.
- Normalized seed labels/indices to `RegionSeed_0` through `RegionSeed_14`, kept BaseType as Nature, and spread State values across existing 0/1/3 usage.
- Added `RT_RegionState_15x1` and set the map's `BP_RegionStateManager` override to `RegionCount=15`.
- Updated `../VoronoIDTextureGen/generate_gamemap1_id_texture.py` to the same 15 seed positions and regenerated the eroded RegionID PNG plus preview.

## Changed Files

- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`
- `Project_Eden/Content/RegionSystem/Blueprints/BP_RegionStateManager.uasset`
- `Project_Eden/Content/RegionSystem/RenderTargets/RT_RegionState_15x1.uasset`
- `../VoronoIDTextureGen/generate_gamemap1_id_texture.py`
- `../VoronoIDTextureGen/T_GameMap1_RegionID_Eroded.png`
- `../VoronoIDTextureGen/T_GameMap1_RegionID_Eroded_preview.png`

## Verification

- Unreal Python readback confirmed active map `/Game/Maps/MainMap/L_LandscapeMap`, 15 seeds, labels, coordinates, BaseType/State values, `RegionCount=15`, and `RT_RegionState_15x1` size `15x1`.
- `EditorLevelLibrary.save_current_level()` returned `True`.
- `git status --short` showed no `L_GameMap` modification; `PCG_Vegetation_Global.uasset` was modified but not intentionally edited in this task.
- Generator run printed `RegionCount: 15`; Pillow verification confirmed `1024x1024 RGB` output with all 15 expected encoded region values in R/G.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
