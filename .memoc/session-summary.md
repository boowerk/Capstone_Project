---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-05-31T15:55:00+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-05-31T15:55:00+09:00
Replace, do not append. Keep <800B.

## Status
- Added `[ActionEndTrace]` logs around ActionEnd, fallback timer, input cancel broadcast, montage complete/blendout/interrupted, EndAbility, StopActionMotionTracking, and StopFallbackMontage.
- `DashSlash` target montage `OnBlendOut` is now log-only, not EndAbility; RM/inertia logic unchanged except logs.
- Logs show no-input fallback roll is not stopped by input; source montage enters auto blend-out/default slot before timer completion.
- Set source fallback RM montage blend-out time to 0 on `AM_UEFN_Roll_RM` and `AM_UEFN_Sword_Dash_RM`.
- User enabled `Always Update Source Pose`; A-pose fixed. Source fallback montage BlendOut time tested at `0.12`, then raised to `0.25` for smoother roll/dash-slash idle handoff.

## Verify
- Live Coding compile command succeeded.
- Assets saved after stopping PIE.
