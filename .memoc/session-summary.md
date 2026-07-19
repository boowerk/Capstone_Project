---
memoc: true
type: state
scope: project-memory
updated: 2026-07-19T19:08:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T13:21:02
---
# Session Summary

## Status
- Region V2.2 direct top-4 blend is ON only for GameMap2. PCG/hard ID/map/other MIs are unchanged; no commit.

## Verified
- Deterministic G8x2 IDs + RGBA8 weights pass 32/32 checks. UE formats pass; 206/206 shaders and zero material errors. OFF restores the V2.1 white network; ON removes it; four-way has no white/black seam.

## Resume
- Roll back with `UseRegionVisualBlendV22=false`. Farm stripes/color contrast and outer-wall stretch remain art/projection work. Backup: `Saved/CodexBackups/RegionVisualV22PreApply`.
