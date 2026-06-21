---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T15:10:00+09:00
status: active
---
# Session Summary

## Status
- Minimap GPU completion gating implemented in `23aedf2b`; regression coverage in `0bfa780f`.
- HUD keeps the old front buffer until the capture's RHI GPU fence write is issued, pending writes drain, and `Poll()` succeeds.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Project_EdenEditor Development Win64 build passed.
- `ProjectEden.UI.Minimap.CaptureStability` passed with NullRHI and real D3D12 offscreen rendering.

## Resume
- PIE-check repeated attacks/Niagara for visual stability. No editor setup is required.
