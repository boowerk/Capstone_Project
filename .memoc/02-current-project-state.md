---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-04T03:45:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Current Project State

Last synced: 2026-06-04T03:45:00+09:00

## Current Status

- Do not run UBT/build while the editor is open. User uses Live Coding for C++ changes.
- `UEFNSourceMesh` is the source animation mesh; `CharacterMesh0`/MaskMan receives pose through parent-based retargeting. Keep `CharacterMesh0` as a child of `UEFNSourceMesh`.
- `ABP_UEFNSource_Player` uses motion matching with a DefaultSlot path. `UGP_CharacterAnimInstance` owns chooser-facing locomotion context such as `MovementMode`, `Stance`, `MovementState`, `Gait`, start/pivot/stop/TIP flags, and graph DB/state variables.
- Primary melee recovery is embedded inside attack montages. `UGP_Primary` should not play separate `_Rec` montages. Primary blocks sprint while active and resumes held sprint when Primary ends.
- Primary movement logs use `[PrimaryMove]`; moving attacks should be checked in PIE for ground/velocity issues, not by UBT.
- `CHT_MM_MaskMan_Root_OriginalStyle` has crouch routing: root rows branch Standing/Crouching by `Stance`; `Crouch Idles` selects Dense crouch TIP vs Idle by `ShouldTurnInPlace`; `Crouch Walks` selects Dense crouch Pivot/Start/Stop/Loop by `IsPivoting`, `IsStarting`, `IsStopping`.
- Hold crouch exists for testing: `IA_Crouch` is mapped to `C`, `AGP_PlayerController` calls `Crouch()`/`UnCrouch()`, `AGP_PlayerCharacter` enables `bCanCrouch`, and `CrouchedHalfHeight` defaults to `64.0f`.
- `UGP_CharacterAnimInstance::ApplyChosenDatabase` must route selected crouch DBs into the correct graph database variable and `CurrentMotionMatchState`, because the ABP graph does not rely on `RuntimePoseSearchDatabase` alone.
- Crouch mesh-position hacks were removed. C++ should leave `BaseTranslationOffset` and mesh relative Z alone during crouch while testing theory. `ABP_MaskMan_Player` Retarget Pose From Mesh source-pin experiment was reverted; keep the pin unlinked and rely on attached parent. `AGP_PlayerCharacter::PostInitializeComponents` reattaches `UEFNSourceMesh` to the capsule if the BP CDO lost its parent, then ensures `CharacterMesh0` is under `UEFNSourceMesh`. Current investigation: default UE crouch capsule-bottom movement vs animation foot/root basis.
- `ABP_UEFNSource_Player` actual output path is MotionMatching -> PoseHistory -> LocomotionPose, not the old state BlendList. The PoseHistory source MotionMatching node was rewired so its `Database` pin uses `RuntimePoseSearchDatabase`; this should allow crouch Chooser-selected PSDs to drive the visible retarget pose.
- `UGP_CharacterAnimInstance::ApplyRuntimeDatabaseToMotionMatchingNode` uses `ForceInterruptAndInvalidateContinuingPose` only when `CurrentMotionMatchState` enters Jump with a changed runtime DB; normal DB changes remain `DoNotInterrupt` for smooth locomotion.

## Open Tasks

- PIE validate that pressing/releasing C visibly plays crouch locomotion, and running jump switches to jump without waiting for run playback.
