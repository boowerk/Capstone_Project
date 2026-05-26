---
memoc: true
type: worklog
scope: project-memory
created: 2026-05-22T14:13:46
updated: 2026-05-22T14:13:46
status: active
tags:
  - memoc
  - memoc/worklog
---
# Add GroundBurst native skill

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-05-22T14:13:46

## Summary

- Added native `UGP_Skill_GroundBurst` ability.
- Registered `GPTags.Cooldown.Skill.GroundBurst` for data-driven cooldown setup.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/AbilitySystem/Abilities/Player/Character1/GP_Skill_GroundBurst.h`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_GroundBurst.cpp`
- `Project_Eden/Source/Project_Eden/Public/GameplayTags/GP_Tags.h`
- `Project_Eden/Source/Project_Eden/Private/GameplayTags/GP_Tags.cpp`

## Verification

- UE_5.7 UBT compiled `GP_Skill_GroundBurst.cpp` and `GP_Tags.cpp`.
- Full build failed at link because running UnrealEditor locked `UnrealEditor-Project_Eden.dll`.

## Follow-up

- Close the editor and rerun full build, then create BP GA/DataAsset and add it to the test preset.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
