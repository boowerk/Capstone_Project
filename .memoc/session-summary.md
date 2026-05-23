---
memoc: true
type: state
scope: project-memory
created: 2026-05-23T05:37:00
updated: 2026-05-23T23:05:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-23T23:05:00
Replace, do not append. Keep <800B.

## Status
- Compared sample/capstone camera, air movement, and acceleration structure.
- `BP_GP_PlayerCharacter` now keeps sample-like air braking/control values.

## Changed
- `/Game/Characters/PlayerCharacter/BP_GP_PlayerCharacter`
  - `AirControl`: `0.35` -> `0.25`
  - `BrakingDecelerationFalling`: `300` -> `1500`
- Camera defaults moved toward sample feel in `AGP_PlayerCharacter`.
- Turn-in-place manual yaw snap was removed in favor of root motion.

## Open Tasks
- Implement Ground Normal Start-only conditional MaxAcceleration clamp.
