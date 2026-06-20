---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T07:05:00+09:00
status: active
---
# Session Summary
Last: 2026-06-21T07:05:00+09:00

## Status
- Minimap opacity fix completed in `c46e718f` + `29bd6b4a`.
- BaseColor alpha made the direct UMG map transparent; capture now uses opaque FinalColorLDR with lighting/shadows/VFX disabled.
- RenderTarget/TextureTarget may remain null in BP; runtime creates and binds a 1024 RT automatically.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Editor build passed.
- `ProjectEden.UI.Minimap.CaptureStability` passed, including production HUD image presence/visibility.

## Resume
- Reopen Editor and PIE-check map contrast.
