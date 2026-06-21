---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T15:53:23+09:00
status: active
---
# Session Summary

## Status
- `a88bfee6` captures the full PCG map once, waits for fenced GPU copy, then disables SceneCapture/tick.
- `b3652834` adds fixed-map UV pan/zoom plus separate player/enemy UMG markers.
- `e6806c69` covers one-shot shutdown, world-to-UV, UI material, and HUD marker host.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build and real D3D12 offscreen minimap automation passed; material compiles cleanly.

## Resume
- Assign `M_UI_Minimap_StaticMap` to HUD `MinimapMapMaterial`; PIE-check orientation, map coverage, zoom, and red enemy points.
