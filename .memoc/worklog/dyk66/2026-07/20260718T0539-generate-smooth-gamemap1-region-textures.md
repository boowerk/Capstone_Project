---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-18T05:39:52
updated: 2026-07-18T19:25:11+09:00
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

- Long ownership stairs are resolved. Remaining follow-up is the thin dotted boundary line plus optional triple-junction and distant-mip polish.

## Applied in Unreal

- Imported Pair-ID and smooth Edge assets under `/Game/RegionSystem/Textures/RegionID/GameMap1_Smoothed`.
- Added `MF_RS_GetRegionBlendData` and a default-false `UseSeparateEdgeTexture` path to `M_StateMask`; enabled it only for `MI_RegionLandscape_GameMap2`.
- Extended `BP_RegionStateManager` and its placed map instance to push the Edge texture. Seven affected assets validated `VALID` with no errors or warnings; PIE showed no material, shader, or Blueprint runtime errors.
- Visual inspection exposed residual stairs. PNG analysis confirmed Edge is present at every ownership seam but drops to 148/255; the current reciprocal `Edge*0.5` blend can therefore jump by up to 41.8% when Nearest R changes.
- Added a 2px fully saturated chamfer seam core with falloff to 8px, composited by maximum over the preserved smooth source Edge. All 7,443 transitions now meet at 255/255 with calculated discontinuity 0.
- Reimported only the Unreal Edge texture. Its grayscale/Bilinear/mipped/clamped settings and `VALID` result remain intact; PIE target errors are zero and the former worst boundary no longer shows a large material stair.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
