---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-19T08:31:17+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-20T00:00:00+09:00
Replace, do not append. Keep <800B.

## Status
- FurnaceWalker: `UPDA_EnemyAnimationSet` built externally. Created `/Game/Characters/EnemyCharacter/Monsters/FurnaceWalker/PDA_FW_EnemyAnimationSet` (mesh, ABP, Idle/Jog, Mutant Punch L/R + Zombie Smashing L/R) and set it on `BP_FurnaceWalker.EnemyAnimationSet`; legacy `AnimationSet` is null. `GP_EnemyAttack` prefers new PDA, with legacy fallback for unmigrated enemies. ABP now has clean Cached Pose branching: stationary full-body DefaultSlot; moving uses a second DefaultSlot layered from `spine_01` (depth 0) over `FW_LocomotionPose`; final blend has 0.2/0.3s internal values and is not MCP-compiled. All four selected attack montages use 0.45s Blend Out. Mutant Punch L/R direct montage events live only on `GameplayEvents`: AttackHit .495, ActionEnd 1.05. No MCP build.
- DragonSkull Control Rig: `CR_DeagonBone_SimpleJaw`; global controls drive root, Head, upper/lower jaw; no scale drive.
- Sans Ground Hands: 3 staggered hands x 3 waves; 0.75s red decal, damage, upward launch.
- Sans summon uses `Basic/BP_BasicEnemy_Melee`.

## Verified
- Editor build and Sans Ground Hands selector passed. PIE decal/timing/collision/launch unverified.

## Resume
- PIE-check FurnaceWalker Idle/Jog, 4 attack random L/R alternation, montage-visible `AttackHit` damage timing, `ActionEnd` ability release, and capsule/mesh offset. Moving-only upper-body split is desired next, but automated CachedPose graph tooling failed; do manually or add a safer editor helper. Keep user-owned MainMap and Matador memo untouched.
