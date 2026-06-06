---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-06T21:30:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Current Project State

Last synced: 2026-06-06T21:30:00+09:00

## Current Status

- Do not run UBT/build while the editor is open. User prefers Live Coding for C++ changes.
- `UEFNSourceMesh` is the source animation mesh. `CharacterMesh0`/MaskMan should stay parented under `UEFNSourceMesh`; avoid reintroducing mesh Z hacks or detached retarget experiments.
- `ABP_UEFNSource_Player` uses motion matching with chooser-selected pose-search DBs, and `UGP_CharacterAnimInstance` owns locomotion context (`MovementMode`, `Stance`, `MovementState`, `Gait`, start/pivot/stop/TIP flags, graph DB state).
- Primary melee currently embeds recovery inside attack montages, blocks sprint while active, forces crouch during the combo, and restores uncrouch only if crouch input is no longer held.
- Skill VFX should be data-driven: montages keep timing events, abilities interpret cues, and `UGP_SkillData.VisualCues` owns Actor/Niagara selection. `UGP_SkillBase` now falls back to `DefaultSkillData` when the spec has no SkillData SourceObject.
- Primary Sword_Light VFX resolves trail/burst Niagara from SkillData cue tags and attaches to visible `GetMesh()` socket `hand_r`; direct Niagara notifies were removed from Sword_Light montages. `GA_Primary.DefaultSkillData` is now set to `/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_Primary`; VisualCues[0]=trail and [1]=burst with CueTags assigned. `UGP_Primary` still hard-fallbacks to that asset as a safety path.
- GameplayCue VFX tags are native-declared in `GP_Tags.h/.cpp` under `GPTags::GameplayCue::Ability` while keeping actual tag names `GameplayCue.Ability.Trail.Magic` and `GameplayCue.Ability.Burst.Magic`; config-only declarations were removed.
- Idle-start primaries stay full-body for the whole combo; moving-start primaries keep lower-body MM except configured combo indices, with moving lower-body blend alpha currently tuned to `0.65`.
- Current crouch/MM work depends on `CHT_MM_MaskMan_Root_OriginalStyle`, `ApplyChosenDatabase`, and the `ABP_UEFNSource_Player` MotionMatching -> PoseHistory -> LocomotionPose path using `RuntimePoseSearchDatabase`.
- Root-motion extraction tuning is in progress in `UGP_RootMotionExtractionEditorLibrary`, including snapped direction (`15` degrees) and foot-plant max-speed scale (`0.85`).
- Camera composition currently targets spring arm lengths `340/380/460` for idle/normal/sprint with socket offset `(0,65,20)`.

## Open Tasks

- PIE validate crouch start/hold/release behavior, visible crouch locomotion, and running jump transition without waiting on run playback.
- Live Coding compile and editor-validate the latest extracted root-motion assets, especially foot-bone sourced extraction.
- Refresh any stale Blueprint pins after the latest animation / root-motion updates.
- PIE validate Primary Sword_Light trail/burst VFX from `DA_Skill_Primary`. See `Project_Eden/Docs/SkillVisualCueStructure.md`.
