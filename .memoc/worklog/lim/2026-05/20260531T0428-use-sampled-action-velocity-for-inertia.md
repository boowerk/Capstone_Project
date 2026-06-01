---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-31T04:28:09
updated: 2026-05-31T04:28:09
status: active
tags:
  - memoc
  - memoc/worklog
---
# Use sampled action velocity for inertia

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-05-31T04:28:09

## Summary

- Added action motion tracking that samples recent actor-location deltas.
- Dash, Primary, and DashSlash now apply inertia from the shared sampled velocity path on normal completion.
- Cancelled abilities discard samples; fallback RM scale multiply remains removed.

## Changed Files

- `memoc/session-summary.md`
- `Project_Eden/Content/Characters/PlayerCharacter/ABP_UEFNSource_Player.uasset`
- `Project_Eden/Content/Characters/PlayerCharacter/BP_GP_PlayerCharacter.uasset`
- `Project_Eden/Content/Characters/UEFN_Mannequin/Animations/Montage/Roll/AM_UEFN_Roll_RM.uasset`
- `Project_Eden/Content/Characters/UEFN_Mannequin/Animations/Montage/Roll/UEFN_Roll_RM.uasset`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_DashSlash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Dash.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/GP_Primary.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_PlayerCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Public/Characters/GP_PlayerCharacter.h`

## Verification

- `Build.bat Project_EdenEditor Win64 Development ... -WaitMutex` succeeded.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
