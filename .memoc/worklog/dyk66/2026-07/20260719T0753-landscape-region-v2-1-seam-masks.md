---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-19T07:53:41
updated: 2026-07-19T07:54:30
status: active
tags:
  - memoc
  - memoc/worklog
---
# Landscape Region V2.1 seam masks

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-19T07:53:41

## Summary

- Added Landscape-only V2.1 SDF Blend and pair-switch/junction masks behind one rollback switch.
- Added a replaceable neutral dirt surface and enabled V2.1 only on the GameMap2 Landscape MI; PCG and the map stayed unchanged.

## Changed Files

- `M_StateMask.uasset`, `MI_RegionLandscape_GameMap2.uasset`
- `MF_RS_TransitionNeutral.uasset`, `T_GameMap1_RegionBlendV21.uasset`, `T_GameMap1_RegionTransitionV21.uasset`
- V2.1 generator/PNGs/report in `Saved/CodexScratch/RegionTransitionV21` and external `VoronoIDTextureGen/.../V2`

## Verification

- Generator validation passed; ordinary seam delta `0.0`, exact core ~0.523m, active coverage 6.634%.
- Unreal compiled 165/165 shaders with no material errors; saved graph export confirmed wiring, and Lit/Unlit OFF/ON checks removed the black line at pair and junction probes.

## Follow-up

- Current neutral dirt is bright/path-like validation art; tune textures or mask width later if desired. Roll back with `UseRegionTransitionV21=false`.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
