---
memoc: true
type: state
scope: project-memory
updated: 2026-06-06T21:30:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary

Last: 2026-06-06T21:30:00+09:00

## Status
- No UBT/build while the editor is open; use Live Coding.
- Reworked Primary/Sword_Light VFX toward SkillData VisualCues: no direct Niagara notifies; Primary resolves trail/burst from SkillData and attaches to `hand_r`.
- After editor reload, set `GA_Primary.DefaultSkillData` to `/Game/GAS_Pattern/AbilitySystem/SkillData/DA_Skill_Primary`; VisualCues[0]=`GameplayCue.Ability.Trail.Magic` -> `NS_ArrowTrail_Magic`, [1]=`GameplayCue.Ability.Burst.Magic` -> `NS_Free_Magic_Slash`.
- Moved VFX GameplayCue tags into native `GP_Tags.h/.cpp` (`GPTags::GameplayCue::Ability::*`) and removed config-only GameplayCue/VFX tag list entries.
- Added `DefaultSkillData` fallback to `UGP_SkillBase` for always-granted abilities without SourceObject.
- Added usage doc: `Project_Eden/Docs/SkillVisualCueStructure.md`.
- `UEFNSourceMesh` remains the source animation mesh and `CharacterMesh0` stays attached under it.
- Current work is motion matching, crouch routing, chooser DB application, and root-motion extraction tuning.

## Open Tasks
- PIE validate crouch locomotion, primary crouch/sprint interaction, and running jump transition.
- Live Coding compile and editor-check the latest root-motion extraction changes.
- PIE validate Primary Sword_Light VFX.

## Resume
- Start with `02-current-project-state.md` and `04-handoff.md`, then verify the live AnimBP / chooser path in PIE.
