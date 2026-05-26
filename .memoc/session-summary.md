---
memoc: true
type: state
scope: project-memory
created: 2026-05-23T05:37:00
updated: 2026-05-24T00:35:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-26T19:33:00
Replace, do not append. Keep <800B.

## Status
- Added source-skeleton fallback montage slots to `PDA_CharacterAnimationSet`.
- `GP_PlayerCharacter` now applies UEFNSource fallback montage root-motion translation via `SafeMoveUpdatedComponent` and caches last RM velocity.
- Dash/Primary use PDA montage first, then UEFNSource fallback montage when empty.
- Fixed source fallback dash hang: `ABP_UEFNSource_Player` now has `DefaultSlot` between Pose History and Output Pose; `GP_Dash` adds a fallback-duration timer so missing ActionEnd notifies cannot leave `Fixed` stuck.
- Added PDA `SourceRootMotionTranslationYawOffset` default `-90` and applies it only to fallback source RM translation.
- Reverted bad experiments: no `UEFNSourceMeshScale` multiplication on consumed RM translation, and AnimInstance speed is back to `Character->GetVelocity()`.
- Reverted bad debug logging experiment; on-screen speed display is back to `GroundSpeed` / `Speed2D`.
- Fixed opposite-direction input linger: MoveAction Completed now resets input/smoothing separately, and move direction smoothing snaps on opposite direction.
- Found speed scale data issue: `PDA_MaskMan_AnimationSet.MovementSpeedProfile.MovementSpeedScaleRatio` was 1.0 while `UEFNSourceMeshScale` was 1.22. Runtime PIE asset/player were set to 1.22 and `UpdateAnimationSet` called; save is blocked during PIE.
- Speed debug now only prints from the locally controlled pawn's `CharacterMesh0`; stale transient worlds and `UEFNSourceMesh` cannot overwrite the same screen-debug key.
- `UGP_CharacterAnimInstance` now reads final `AGP_PlayerCharacter::GetMovementSpeedScaleRatio()` and `AGP_PlayerCharacter` pushes that ratio into both target/source anim instances when profiles or GAS scale change.
- Runtime PIE verify: active player `CharacterMesh0`/`UEFNSourceMesh` scale=1.22, raw=427, MM=350; stale `/Engine/Transient.World_0` remains player=false and is ignored by debug.
- `PoseSearchChooser` default load/evaluation is gated to the UEFNSource anim instance only; target `ABP_MaskMan_Player`/boss no longer feed the source-only chooser after Live Coding patch_5.
- Opposite-direction start acceleration clamp now skips when current velocity and acceleration oppose, reducing reversal linger.

## Changed
- `PDA_CharacterAnimationSet.h`
- `GP_PlayerCharacter.{h,cpp}`
- `GP_Dash.{h,cpp}`
- `GP_Primary.cpp`
- `ABP_UEFNSource_Player.uasset`

## Open Tasks
- User re-test dash/reversal in PIE after patch_5.
- Full UBT build still needs editor/Live Coding closed; Live Coding compile succeeded.
