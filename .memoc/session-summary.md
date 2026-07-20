---
memoc: true
type: state
scope: project-memory
updated: 2026-07-17T15:02:00+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-17T06:03:26
---
# Session Summary

## Status
- FurnaceWalker attack is now one synchronized pair: `AM_FW_Attack` upper montage + `A_FW_Sword_Attack_RM` lower root motion.
- Basic-enemy cadence starts after ability recovery/end, not activation; `bBasicEnemyAttackInProgress` also locks BT chase/reposition during the full attack. Missing hit notifies use a 0.55s fallback rather than recovery-end damage.

## Verified
- `Project_EdenEditor Win64 Development` build passed.
- Editor MCP reloaded `PDA_FW_EnemyAnimationSet` with the intended one-to-one montage/root mapping.

## Handoff
- PIE-check FurnaceWalker: approach → prepare/strike/recover → cooldown → reattack or chase.
- Confirm `AM_FW_Attack` has `Enemy.AttackHit` at its hit frame and `Enemy.ActionEnd` after recovery.
