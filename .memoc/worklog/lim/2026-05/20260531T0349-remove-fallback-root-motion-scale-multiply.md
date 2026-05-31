---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-31T03:49:29
updated: 2026-05-31T03:49:29
status: active
tags:
  - memoc
  - memoc/worklog
---
# Remove fallback root motion scale multiply

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-05-31T03:49:29

## Summary

- Removed direct `MovementSpeedScaleRatio` multiply from fallback root motion translation.
- Keeps inertia disabled so target/fallback montage distances can be compared without handoff noise.

## Changed Files

- `memoc/session-summary.md`
- `Project_Eden/Content/Characters/MaskMan/PDA_MaskMan_AnimationSet.uasset`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_DashSlash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Dash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Primary.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Public/Characters/GP_PlayerCharacter.h`

## Verification

- Full UBT blocked by active Live Coding.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
