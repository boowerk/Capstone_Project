---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-19T23:46:00+09:00
updated: 2026-07-20T15:47:38+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# Randomize Landscape Region V2.2 boundaries

actor: dyk66
actor_source: OS user
branch: feature/vfx-skills
status: done
created: 2026-07-19T23:46:00+09:00

## Summary

- Committed the prior stable PCHIP checkpoint as `7287602f` before starting this pass.
- Replaced normalized coarse/detail waves with Candidate J: one non-uniform-anchor PCHIP per movable region-pair boundary using 36-260m spacing, 4-26m amplitudes, 50% sign persistence, and a 16m cap.
- Added an independent continuous 18-32px blend half-width field per region pair with 80-240m anchors; each region samples its own nearest boundary-side pixel while junctions, endpoints, and the perimeter return to 24px.
- Kept hard RegionID, packed visual IDs, PCG, StateRT, material topology, and the production map unchanged.

## Verification

- Generator passes 61/61 and produces identical hashes in two full runs.
- Weight PNG is `8209CBD4...11917`; IDs01/IDs23 remain `5FAD10BB...E0B7` / `7A0E2CBF...0913`.
- Observed blend half-width is 18.105-31.112px (mean 23.935px) with actual anchor gaps 81.045-237.979m. Unassigned boundary pixels, same-slot support/guard overlap, active-ID mismatch, ID-transition violations, topology changes, and protected-junction changes are zero; maximum active regions is four.
- Computer Use reimported Weight and saved it as regular BGRA8, Bilinear, NoMipmaps/one mip, Clamp X/Y, sRGB off, VectorDisplacementmap (`F91AB7E1...53CC6`). `MI_RegionLandscape_GameMap2` remains saved with `UseRegionVisualBlendV22=true` (`841EE208...D740`), and no new material/sampler error followed.
- A uniform beige follow-up screenshot was traced to black transient `RT_RegionState_15x1`, not an absent V2.2 material or Weight. The Landscape MI, V2.2 switch, Weight binding, and non-VT texture settings were all correct. `DebugRandomRegionStates` plus `RefreshRegionTexture` immediately repopulated the RT and restored distinct material regions. The current map file's 01:18 mtime predates this 02:26+ diagnosis; no map save was issued in this pass.
- Repository source and external `VoronoIDTextureGen/.../V2.2` mirrors match. The user-owned `L_LandscapeMap` working copy was not saved; its hash remains `E8CE0B5D...B728C`.

## Follow-up

- Inspect the new irregular geometry and variable softness at player height. Farm stripes, four-state material repetition, strong per-region tiling, and steep outer-wall projection stretch remain separate art/projection tasks.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
