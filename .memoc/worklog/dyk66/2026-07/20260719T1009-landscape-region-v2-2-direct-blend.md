---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-19T10:09:28
updated: 2026-07-19T10:09:28
status: active
tags:
  - memoc
  - memoc/worklog
---
# landscape-region-v2-2-direct-blend

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-19T10:09:28

## Summary

- Added the GameMap2-only V2.2 visual top-4 region blend while preserving hard RegionID, PCG, V2.1, and V2 rollback paths.
- Replaced the rejected G16 design with two float16-exact G8 nibble-ID textures plus one RGBA8 weight texture.

## Changed Files

- `M_StateMask.uasset`, `MI_RegionLandscape_GameMap2.uasset`
- `T_GameMap1_RegionVisualIDs01V22.uasset`, `T_GameMap1_RegionVisualIDs23V22.uasset`, `T_GameMap1_RegionVisualWeightsV22.uasset`
- Generator, validation report, backup, and Unreal helper scripts under `Saved/`
- Mirrored generator, PNGs, and report under external `VoronoIDTextureGen/GameMap1_RegionID_Smoothed/V2.2`

## Verification

- Deterministic generator: 32/32 checks, exact G8 float16 decode, no topology/slot/weight errors.
- Unreal: strict format readback, 206/206 shaders, zero material errors, complete graph/MI post-audit.
- OFF/ON aerial and four-way QA restored/removed the V2.1 white network; map and gameplay/PCG texture hashes stayed unchanged.

## Follow-up

- Tune incomplete regional materials/farm stripes and steep outer-wall projection separately; rollback is `UseRegionVisualBlendV22=false`.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
