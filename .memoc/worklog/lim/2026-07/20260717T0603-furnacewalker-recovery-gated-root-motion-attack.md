---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-17T06:03:26
updated: 2026-07-17T06:03:26
status: active
tags:
  - memoc
  - memoc/worklog
---
# FurnaceWalker recovery-gated root-motion attack

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-17T06:03:26

## Summary

- FurnaceWalker now plays `AM_FW_Attack` with paired `A_FW_Sword_Attack_RM` lower-body root motion.
- Regular-enemy cadence starts only after attack recovery/ability completion; missing hit notifies fall back at 0.55s.

## Changed Files

- `.memoc/02-current-project-state.md`
- `.memoc/session-summary.md`
- `Project_Eden/Content/Characters/EnemyCharacter/Monsters/FurnaceWalker/PDA_FW_EnemyAnimationSet.uasset`
- `Project_Eden/Source/Project_Eden/Private/AI/Tasks/BTT_ExecuteEnemyAttack.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Enemy/GP_EnemyAttack.cpp`
- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/Enemy/GP_EnemyAttack.h`

## Verification

- `Project_EdenEditor Win64 Development` build passed.
- Editor MCP reloaded the expected one-to-one FurnaceWalker montage/root mapping.
- `ProjectEden.AI.Enemy.AttackCadence` was queued in the editor; no completion result was emitted in the log.

## Follow-up

- PIE-check `Enemy.AttackHit` and `Enemy.ActionEnd` notify placement in `AM_FW_Attack`.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
