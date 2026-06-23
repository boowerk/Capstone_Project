---
memoc: true
type: state
scope: project-memory
updated: 2026-06-24T00:13:20+09:00
status: active
created: 2026-06-22T17:59:08
tags:
  - memoc
  - memoc/state
---
# Session Summary

## Status
- Minimap map material now uses a translucent circular opacity mask, keeping the captured map inside the brass ring.
- Player minimap arrow now follows controller/view yaw by default, so camera-facing an enemy aligns the cursor more closely with the target marker.

## Verified
- `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.UI.Minimap.CaptureStability` automation succeeded.

## Handoff
- PIE-check ring fit and, if needed, tune `MinimapCircleMaskRadius` or `MinimapPlayerArrowAngleOffset` on `WBP_PlayerHUDWidget`.
