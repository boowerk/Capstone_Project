---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-18T11:24:59
updated: 2026-07-18T11:24:59
status: active
tags:
  - memoc
  - memoc/worklog
---
# Rebuild Landscape Region textures at 4096

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-18T11:24:59

## Summary

- Rebuilt authoritative Landscape Region Pair/Edge textures at 4096 with SDF top-2 reconstruction and reimported them into Unreal.
- Disabled auto Virtual Texture conversion and retained exact-ID/filtered-edge sampling rules.

## Changed Files

- `Project_Eden/Content/RegionSystem/Textures/RegionID/GameMap1_Smoothed/T_GameMap1_RegionID_Pair.uasset`
- `Project_Eden/Content/RegionSystem/Textures/RegionID/GameMap1_Smoothed/T_GameMap1_RegionEdge_Smooth.uasset`
- External non-Git generator and PNG outputs under `VoronoIDTextureGen/GameMap1_RegionID_Smoothed`.

## Verification

- Generator invariants: 15 connected regions, 32 adjacency pairs, aligned source samples, max area drift 0.000071526pp.
- Unreal final readback: both 4096/non-VT; Pair Nearest/NoMip, Edge Bilinear/mipped; material recompile succeeded.
- PIE rendered adjacent Regions 0/1 in contrasting states without the former repeated 1m staircase.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
