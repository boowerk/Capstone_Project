---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-23T07:38:17
updated: 2026-05-23T07:38:17
status: active
tags:
  - memoc
  - memoc/worklog
---
# Tune player jump inertia

actor: lim
actor_source: git config user.name
branch: feature/motion-matching
status: done
created: 2026-05-23T07:38:17

## Summary

- Checked `BP_GP_PlayerCharacter` CDO CharacterMovement values after run-jump felt like it lacked inertia.
- Tuned air movement defaults: `AirControl 0.2 -> 0.35`, `BrakingDecelerationFalling 1500 -> 300`.

## Changed Files

- `memoc/00-agent-index.md`
- `.memoc/00-project-brief.md`
- `.memoc/01-agent-workflow.md`
- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/04-handoff.md`
- `.memoc/05-done-checklist.md`
- `.memoc/session-summary.md`
- `.memoc/wiki/index.md`
- `CHT_MM_MaskMan_Root_OriginalStyle.md`
- `Project_Eden/Content/Characters/PlayerCharacter/BP_GP_PlayerCharacter.uasset`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`

## Verification

- Re-inspected the Blueprint CDO and confirmed the updated CharacterMovement values were saved.
- PIE runtime feel still needs manual re-test.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
