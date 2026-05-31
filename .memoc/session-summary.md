---
memoc: true
type: state
scope: project-memory
created: 2026-05-26T11:35:43
updated: 2026-05-28T20:13:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-31T05:45:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- Tech element drives VFX/damage.
- SkillData has actor/VFX fields + `SkillIdTag` (`GPTags.Ability.Skill.Id.*`).
- Augment DA shell exists; PlayerState replicates selected augments.
- Augment `GrantedElementTag` applies current tech.

## Open Tasks
- Rebuild; fill each SkillData `SkillIdTag`.
- Fill DA actor/VFX for ThrownBurst/SplitShot/MineBurst.
- Connect augment modifiers.
- Later replace raw `K` with Enhanced Input `IA_ToggleTechSelect`.
