---
memoc: true
type: state
scope: project-memory
status: active
tags:
  - memoc
  - memoc/state
updated: 2026-05-31T15:01:23+09:00
created: 2026-05-28T12:18:30
---
# Session Summary
Last: 2026-05-31T15:01:23+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Action speed formula: target = target RM + carry; fallback = runtime-retarget RM + same carry.
- Fallback consumed source RM is multiplied by `GetMovementSpeedScaleRatio()`; carry is not scaled again.
- Added `FallbackRootMotionDistanceCorrection=1.18` on `AGP_PlayerCharacter`; fallback RM total scale is now MovementSpeedScaleRatio * correction.
- `BeginActionMotionTracking()` captures entry XY velocity, then zeros CMC XY so fallback cannot consume old velocity twice.
- Shared carry uses `SafeMoveUpdatedComponent` for both paths; handoff applies remaining carry only, not sampled RM+carry.
- Logs split `[ActionRM][Sample]` into `RMSpeed` and `CarrySpeed`; fallback also logs `[ActionRM][FallbackTick]`.
- UEFNSource anim debug speed uses `GetCurrentActionMotionVelocity()` during fallback montages so Raw Speed reflects manual RM movement.
- Roll RM distances inspected: target sequence 593.9, source sequence 465.5; PDA scale 1.22 predicts fallback ~568.

## Verify
- Live Coding compile succeeded.
- Live Coding compile succeeded after correction. PIE retest: standing fallback was 502 before correction, expected ~592 after correction; fallback log should show `Scale=1.440 Correction=1.180`.
