---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-19T12:02:21
updated: 2026-07-19T22:20:00+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Apply Landscape Region V2.2 boundary-only wobble

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-19T12:02:21

## Summary

- Replaced the rejected global 2D grid warp with deterministic, independent PCHIP normal displacement for each boundary pair/component.
- Used 2.5m coarse displacement at 42-58m spacing plus 0.65m detail at 22-32m, capped at 3.2m; pinned junction/perimeter cores and faded offsets over 15m.
- Reimported and saved the updated Weight texture through Computer Use, then disabled Virtual Texture Streaming and restored NoMipmaps. Hard ID, PCG, StateRT, map, and topology remain unchanged.

## Changed Files

- `Project_Eden/ArtSource/RegionVisualSlotsV22/`
- `Project_Eden/Content/RegionSystem/Textures/RegionID/GameMap1_Smoothed/V2/T_GameMap1_RegionVisualWeightsV22.uasset`
- External mirror `VoronoIDTextureGen/GameMap1_RegionID_Smoothed/V2.2/` (byte-identical generator, report, and three PNGs)
- Landscape V2.2 project-memory state/decision/handoff files

## Verification

- Generator validation: 42/42 checks pass; 87,666 labels changed (0.52253%), maximum commanded/observed displacement 2.996m/2.828m, 18 junction anchors unchanged, and boundary-halo/proposal/ID-transition violations all zero.
- The 31 safe boundary components are warped independently; the approximately 1.08m Region 1-13 edge stays fixed. All 15 region components and 32 adjacencies remain unchanged.
- Final Weight PNG hash is `4C6A05A9...E75A42E`; saved uasset hash is `E851B002...679B73`. Unreal shows regular non-VT, Bilinear, NoMipmaps/one mip, Clamp, sRGB off.
- The initial VT sampler failure remained cached after conversion and made the Landscape use the default gray material. Toggling `UseRegionVisualBlendV22` false/true forced 13/13 shaders to compile without a new sampler error. Saved `MI_RegionLandscape_GameMap2` with the switch true (hash `9673F631...F049B`); the production map file was not saved.

## Follow-up

- Farm stripes/tiling/color, four-state repetition, and steep outer-wall stretch remain separate material/projection tasks.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
