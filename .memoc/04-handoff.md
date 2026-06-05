---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-06T00:00:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Agent Handoff

Last synced: 2026-06-06T00:00:00+09:00

## Focus Now

- Active area is player locomotion / primary attack / crouch integration around motion matching and chooser-selected DB routing.
- Keep `UEFNSourceMesh` as the source mesh and `CharacterMesh0` under it. Do not revive mesh-offset hacks or source-pin retarget experiments unless explicitly re-testing them.
- Prefer Live Coding plus PIE/editor validation over UBT builds while the editor is open.

## What Matters Most

- `UGP_Primary` currently stops sprint, forces crouch at combo start, preserves crouch while active, and restores uncrouch only if crouch input is not still held.
- Moving primaries use lower-body MM with montage/MM blending (`MovingAttackLowerBodyMotionMatchBlendAlpha = 0.65`) except configured full-body combo indices.
- `UGP_CharacterAnimInstance::ApplyChosenDatabase` still matters because the ABP graph does not rely on `RuntimePoseSearchDatabase` alone.
- `ABP_UEFNSource_Player` output path is MotionMatching -> PoseHistory -> LocomotionPose, and crouch validation should happen against that live path.
- Root-motion extraction tuning is mid-stream; latest intent is snapped direction plus scaled inferred foot-plant speed, followed by Live Coding/editor validation.

## Next Steps

- PIE validate crouch locomotion, crouch release behavior after primary, and running jump transition.
- Live Coding compile and validate the newest root-motion extraction changes and resulting assets.
- Refresh or reconnect any stale Blueprint pins after compile if the editor reports broken animation graph references.

## Verified

- `AGENTS.md` and `.memoc` are in the upgraded memoc layout with actors/worklog/raw/wiki/runtime structure.
- `session-summary.md`, `02-current-project-state.md`, and `04-handoff.md` now reflect the current locomotion / crouch / root-motion investigation instead of older skill-augment and boss-UI work.
- `memoc doctor` previously only warned that `session-summary.md` exceeded 800B.

## Not Verified

- PIE/runtime validation for crouch locomotion, jump transition, and primary crouch/sprint interaction after the latest animation changes.
- Live Coding compile and asset validation for the newest root-motion extraction tuning.

## Suggested Reads

- `.memoc/session-summary.md`
- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/06-project-rules.md`
