---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-19T04:29:09
updated: 2026-07-19T04:29:09
status: active
tags:
  - memoc
  - memoc/worklog
---
# Complete Landscape Region V2 seam fix

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-19T04:29:09

## Summary

- Fixed the V2 hard seam by wiring canonical A directly into the state lookup before Owner/Other reordering.
- Enabled/saved Landscape-only V2 plus the existing slope overlay; PCG and other MIs remain legacy.
- Updated and copied the deterministic 4096 Pair/Blend generator outputs to external `VoronoIDTextureGen/.../V2`.

## Changed Files

- `MF_RS_GetRegionBlendData.uasset`, `M_StateMask.uasset`, `MI_RegionLandscape_GameMap2.uasset`, `MF_RegionGround_Grassland5.uasset`, and the legacy/V2 region textures.
- The external generator remains outside Git under `VoronoIDTextureGen/.../V2`; Unreal helpers, exports, and backups remain untracked under `Saved`.

## Verification

- Re-exported graph proves `Round_0 -> Add_0.A`; 132/132 shaders completed without related errors.
- Lit/Unlit long-seam and complex-junction checks pass; map reloaded clean without saving.
- Generator passes all 15 checks; deterministic Pair/Blend hashes are `748b2e8a...` / `5e1a3c97...`.

## Follow-up

- Optional separate investigation: steep outer-wall projection stretch; true triple-junction fallback is localized to about 0.52m.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
