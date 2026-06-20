---
memoc: true
type: state
scope: project-memory
updated: 2026-06-21T07:42:00+09:00
status: active
---
# Session Summary

## Status
- Fixed attack-time minimap flicker in `d64bc60c`; regression coverage is `88d85765`.
- UMG displays a stable front RT while SceneCapture fills a separate back RT; promotion waits for an RHI fence.
- Followed player is hidden from minimap capture because the HUD arrow represents it.
- User-owned `L_MainMap.umap` remains untouched/uncommitted.

## Verified
- Project_EdenEditor Development Win64 build passed.
- `ProjectEden.UI.Minimap.CaptureStability` passed.

## Resume
- Reopen Editor and PIE-check repeated attacks with production VFX. No editor setup required.
