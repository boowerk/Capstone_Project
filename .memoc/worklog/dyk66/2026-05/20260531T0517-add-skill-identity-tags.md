---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-31T05:17:29
updated: 2026-05-31T05:17:29
status: active
tags:
  - memoc
  - memoc/worklog
---
# add skill identity tags

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-31T05:17:29

## Summary

- Added native `GPTags.Ability.Skill.Id.*` tags for current player skills.
- Added `UGP_SkillData.SkillIdTag`.
- Scoped augment target tags to skill id tags.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/GameplayTags/GP_Tags.h`
- `Project_Eden/Source/Project_Eden/Private/GameplayTags/GP_Tags.cpp`
- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/GP_SkillData.h`
- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/GP_SkillAugmentData.h`

## Verification

- `git diff --check` on changed source files.
- Build not run.

## Follow-up

- Fill each SkillData DA `SkillIdTag` in editor after rebuild.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
