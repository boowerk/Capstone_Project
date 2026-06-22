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
- FurnaceStomper: created `/Game/Characters/EnemyCharacter/Monsters/FurnaceStomper/BP_FurnaceStomper`, `ABP_FurnaceStomper`, `PDA_FS_EnemyAnimationSet`, and 4 montages. PDA has Stomper mesh/ABP/Idle/Jog, attack order Mutant Punch L, Zombie Smashing R, Mutant Punch R, Zombie Smashing L; all `LowerBodyRootMotionSequences` initially use `A_FS_Sword_Attack_RM`. Montages have GameplayEvents (punch .495/1.05; smash 1.63/3.45) + 0.45 Blend Out. ABP duplicates Walker Cached Pose / Enemy_LowerBody setup and was not compiled.
- FurnaceWalker: `PDA_FW_EnemyAnimationSet` assigned to BP; legacy AnimationSet null. ABP Cached Pose: stationary full-body DefaultSlot; moving upper-body DefaultSlot from spine_01 over locomotion. Added `Enemy_LowerBody` slot as moving layered base. New unbuilt PDA field `LowerBodyRootMotionSequences` pairs by LightAttack index; EnemyAttack dynamically plays it in Enemy_LowerBody, and enemy anim instance uses RootMotionFromMontagesOnly. Attack range lowered 120/140 -> 96/112. After user builds, fill 4 entries (currently `A_FW_Sword_Attack_RM` exists) and PIE-check Root Motion. Four attacks use 0.45 Blend Out; Mutant Punch direct events in GameplayEvents. No MCP build.
- DragonSkull Control Rig: `CR_DeagonBone_SimpleJaw`; global controls drive root, Head, upper/lower jaw; no scale drive.
- Sans Ground Hands: 3 staggered hands x 3 waves; 0.75s red decal, damage, upward launch.
- Sans summon uses `Basic/BP_BasicEnemy_Melee`.

## Verified
- Editor build and Sans Ground Hands selector passed. PIE decal/timing/collision/launch unverified.

## Resume
- PIE-check FurnaceWalker Idle/Jog, 4 attack random L/R alternation, montage-visible `AttackHit` damage timing, `ActionEnd` ability release, and capsule/mesh offset. Moving-only upper-body split is desired next, but automated CachedPose graph tooling failed; do manually or add a safer editor helper. Keep user-owned MainMap and Matador memo untouched.
