---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T09:13:22+09:00
status: active
---
# Session Summary

## Status
- Non-Sans bosses use `Boss Telegraph VFX` plus per-pattern BP maps.
- Master `Telegraph VFX On/Off` alone does nothing; checked `Telegraph VFX Patterns` tags get the cue/delay.
- Dark charge skips its internal cue only when Charge is checked.

## Editor
- Select `Boss Telegraph VFX`, assign System Asset, tune duration/scale, enable master bool, then check desired tags in Class Defaults.
- PIE-check timing/placement; groggy and Crystal teleport intentionally bypass it.

## Verified
- Editor build and telegraph configuration + legacy charge automation pass.
- Preserve user changes in `DefaultEditor.ini`, `L_MainMap.umap`, and `WBP_PlayerHUDWidget.uasset`.
