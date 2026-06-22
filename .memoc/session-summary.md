---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T03:47:00+09:00
status: active
created: 2026-06-22T17:59:08
tags:
  - memoc
  - memoc/state
---
# Session Summary

## Status
- `/Game/Maps/MainMap/L_LandscapeMap` has 15 `BP_RegionSeed` actors at the requested SEEDS_WORLD positions; labels/indices remain `RegionSeed_0` through `RegionSeed_14`.
- Existing seed Z heights were preserved: seeds 12/13 remain elevated, all others stay at Z=1000.
- `BP_RegionStateManager` variables `RegionCount` and `StateRT` are now instance-editable so the L_LandscapeMap manager can override `RegionCount=15` and use `RT_RegionState_15x1`.
- `../VoronoIDTextureGen/generate_gamemap1_id_texture.py` was updated to the 15 L_LandscapeMap seed positions and regenerated `T_GameMap1_RegionID_Eroded.png` plus preview.

## Verified
- Unreal Python readback confirmed active map `/Game/Maps/MainMap/L_LandscapeMap` and all 15 seed X/Y coordinates match the user-provided SEEDS_WORLD list.
- Saved L_LandscapeMap after readback; `git status` showed no `L_GameMap` modification.
- PNG verification confirmed `T_GameMap1_RegionID_Eroded.png` is `1024x1024 RGB` and R/G channels contain the 15 expected encoded IDs `[0, 18, 36, 55, 73, 91, 109, 128, 146, 164, 182, 200, 219, 237, 255]`.

## Resume
- `Project_Eden/Content/RegionSystem/PCG/PCG_Vegetation_Global.uasset` appears modified in git status but was not intentionally edited during the RegionSeed work.
