---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-18T05:39:52
updated: 2026-07-18T05:39:52
status: active
tags:
  - memoc
  - memoc/worklog
---
# Generate source-preserving GameMap1 region textures

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-18T05:39:52

## Summary

- Replaced the preliminary domain-warp output with a source-preserving generator under `VoronoIDTextureGen/GameMap1_RegionID_Smoothed`.
- Preserved `T_GameMap1_RegionID.png` R/G channels byte-for-byte and smoothed only the separately emitted B-channel edge mask.
- Removed the four preliminary root-level `*Smooth*` files and generated ID pair, edge, topology preview, and blend preview PNGs.

## Changed Files

- `../VoronoIDTextureGen/GameMap1_RegionID_Smoothed/generate_from_existing_region_id.py`
- `../VoronoIDTextureGen/GameMap1_RegionID_Smoothed/T_GameMap1_RegionID_Pair.png`
- `../VoronoIDTextureGen/GameMap1_RegionID_Smoothed/T_GameMap1_RegionEdge_Smooth.png`
- `../VoronoIDTextureGen/GameMap1_RegionID_Smoothed/T_GameMap1_RegionTopology_preview.png`
- `../VoronoIDTextureGen/GameMap1_RegionID_Smoothed/T_GameMap1_RegionBlend_preview.png`

## Verification

- Generator completed with all 15 primary regions and verified byte-identical R/G topology.
- Source `T_GameMap1_RegionID.png` remained SHA-256 `8aab580e...4c14dd3b`; all target files matched staging hashes.
- Topology, blended preview, and grayscale edge mask were visually inspected.

## Follow-up

- Unreal import/material wiring is still required; ID uses Nearest/NoMip and Edge uses linear filtering/mips with `Edge.R * 0.5` blend alpha.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
