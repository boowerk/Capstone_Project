---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T15:10:00+09:00
status: active
---
# Session Summary

## Status
- `42b72f57` keeps UMG bound to one stable minimap RenderTarget.
- SceneCapture writes only to a back buffer; capture-fence completion queues a GPU copy, and the copy fence blocks back-buffer reuse.
- `19340f27` covers incomplete-copy recapture/rebind prevention.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Project_EdenEditor Development Win64 build passed.
- `ProjectEden.UI.Minimap.CaptureStability` passed with NullRHI and real D3D12 offscreen rendering.

## Resume
- PIE-check repeated attack Niagara; no editor setup is required.
