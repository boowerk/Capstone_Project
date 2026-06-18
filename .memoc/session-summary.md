---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-19T07:20:38+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-19T07:20:38+09:00
Replace, do not append. Keep <800B.

## Status
- `BP_BasicEnemy_Ranged` launches a native projectile toward Blackboard `TargetActor` while retaining shared BT/GAS timing and its 850 cm combat band.
- Aim is recalculated from the elevated spawn point to the player capsule center, preventing shots from passing overhead.

## Verified
- Full `Project_EdenEditor` Development build passed.
- PIE trajectory/damage remain unverified.

## Commits
- `ab0171f4`, `d526e4f9`.

## Resume
- User-owned untracked Matador memo remains untouched.
