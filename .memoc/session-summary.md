---
memoc: true
type: state
scope: project-memory
updated: 2026-06-23T08:45:06+09:00
status: active
---
# Session Summary

## Status
- Non-Sans boss attack boundaries now honor `Boss Telegraph VFX`.
- Crystal/Matador inherit it; Dark Knight reuses its BP component; Sans has none.
- Enabled cues wait Telegraph Duration; Dark charge avoids replaying its internal cue.

## Editor
- Select `Boss Telegraph VFX`, assign System Asset, tune duration/scale, and enable `Telegraph VFX On/Off`.
- PIE-check timing/placement; groggy and Crystal teleport intentionally bypass it.

## Verified
- Editor build and telegraph configuration + legacy charge automation pass.
- Preserve user changes in `DefaultEditor.ini`, `L_MainMap.umap`, and `WBP_PlayerHUDWidget.uasset`.
