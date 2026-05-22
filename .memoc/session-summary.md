---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-21T07:03:24
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-22T17:31:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- MaskMan locomotion now uses camera-facing player rotation: controller yaw drives character facing; orient-to-movement is off.
- Actual directional movement speed scaling moved into `GP_PlayerController::Input_Move`, not just animation thresholds.
- `BP_GP_PlayerController` CDO defaults: forward 500, side 350, back 300; sprint forward 700, sprint side/back 350/300.
- `BP_GP_PlayerCharacter` has `MovementSpeedScaleRatio=1`; actual max speed multiplies by it while AnimBP `Speed2D` divides by it for scale-1 chooser logic.

## Changed
- `BP_GP_PlayerCharacter` CDO keeps `NormalWalkSpeed=500`, `SprintSpeed=700`, mesh scale 1.
- Live Coding compile succeeded after the controller directional speed edit.

## Open Tasks
- PIE-check actual lateral/back speeds and chooser PSD selection after directional/scaled speed changes.
- Tune walk-mode separately only if a real walk input/state is added.

## Resume
- Start with `GP_PlayerController::Input_Move` for real movement speed issues; animation thresholds alone are not enough.
