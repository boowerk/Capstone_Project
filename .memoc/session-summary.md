---
memoc: true
type: state
scope: project-memory
created: 2026-06-06T06:43:32
updated: 2026-06-10T00:00:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-10T00:00:00+09:00

## Status
- Suspected added action inertia as source of multiplayer movement jitter.

## Changed
- `AGP_PlayerCharacter` disables direct action-inertia movement outside standalone.
- `UpdateActionCarryVelocity`, `UpdatePrimaryAttackMovementAssist`, and `ApplyCurrentActionInertia` skip/clear inertia in multiplayer.
- `gp.ActionInertia.Debug` default is now off.

## Resume
- Compile with Live Coding/editor and PIE net-test. If smooth, reimplement action inertia as predicted/replicated movement instead of Tick `SafeMoveUpdatedComponent`.
