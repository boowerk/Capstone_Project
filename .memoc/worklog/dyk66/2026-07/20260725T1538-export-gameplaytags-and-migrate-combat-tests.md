---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-25T15:38:16
updated: 2026-07-25T15:38:16
status: active
tags:
  - memoc
  - memoc/worklog
---
# Export GameplayTags and migrate combat tests

actor: dyk66
actor_source: OS user
branch: unknown
status: done
created: 2026-07-25T15:38:16

## Summary

- Exported all 169 native GameplayTag declarations across the Project_Eden DLL boundary.
- Added direct GameplayAbilities/GameplayTags test dependencies and moved five unchanged tag-dependent combat/AI test files into `Project_EdenTests`.
- Kept the failing Telegraph asset-contract test in the runtime module.

## Changed Files

- `Project_Eden/Source/Project_Eden/Public/GameplayTags/GP_Tags.h`
- `Project_Eden/Source/Project_EdenTests/Project_EdenTests.Build.cs`
- Five combat/AI test source locations

## Verification

- `Project_EdenEditor` and `Project_EdenServer` Development builds: passed.
- Sixteen exact moved automation tests: passed 16/16.
- Editor DLL exports GPTags symbols; Server target contains no `Project_EdenTests`.

## Follow-up

- Decide the failing Dark Knight Charge telegraph content contract separately.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/dyk66.md)
