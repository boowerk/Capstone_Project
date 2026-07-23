---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-23T15:19:48+09:00
updated: 2026-07-23T15:19:48+09:00
status: in-progress
tags:
  - memoc
  - memoc/worklog
  - landscape
  - material
---
# Add landscape macro variation

- Added one shared world-space macro-noise sample to `M_StateMask` after the V2.2 ground result and before the slope/cliff overlay.
- The branch varies BaseColor only and passes the remaining fixed Material Attributes through unchanged.
- Enabled it only on `MI_RegionLandscape_GameMap2` at 180m scale and 0.16 strength.
- Converted `T_RegionGround_MacroNoise_1024` to linear data (`sRGB=false`).
- Added idempotent editor automation in `ApplyRegionMacroVariation.py`.
- UE commandlet verification passed with no related material compile errors.
- Remaining: visually inspect `L_LandscapeMap` after Computer Use reconnects.
