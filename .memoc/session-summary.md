---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T06:40:00+09:00
status: active
---
# Session Summary
Last: 2026-06-21T06:40:00+09:00

## Status
- Minimap fix completed in `2641dbab` + `a0a1a7d8`.
- FullMap no longer adds CaptureHeight every 0.2s; PCG refresh preserves Follow mode and HUD brush size.
- Capture is orthographic BaseColor without lighting, shadows, VFX, decals, or debug overlays.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build passed.
- `ProjectEden.UI.Minimap.CaptureStability` passed.

## Resume
- PIE-check map contrast and selectively re-enable decals only if required for navigation markings.
