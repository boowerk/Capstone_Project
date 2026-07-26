---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-26T16:17:28
updated: 2026-07-26T16:17:28
status: active
tags:
  - memoc
  - memoc/worklog
---
# Fix Matador Bull, Dark Wave, and party friendly fire

actor: douyun0623
actor_source: git config user.name
branch: refactor/codebase-cleanup
status: done
created: 2026-07-26T16:17:28

## Summary

- Raised Bull spawn clearance, centered the Colosseum Matador point, and added Dark Wave spawn/replication diagnostics.
- Routed single-phase boss-zone batches through the dedicated boss point/center path while preserving portal and recovery placement behavior.
- Blocked co-op player friendly fire at effect filtering and damage execution, including Life Drain's direct-spec path.

## Changed Files

- `memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/04-handoff.md`
- `.memoc/session-summary.md`
- `Project_Eden/Content/Maps/MainMap/L_LandscapeMap.umap`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/Abilities/Player/Character1/GP_Skill_LifeDrainTarget.cpp`
- `Project_Eden/Source/Project_Eden/Private/AbilitySystem/GP_DamageExecCalculation.cpp`
- `Project_Eden/Source/Project_Eden/Private/Actors/GP_DarkWaveProjectile.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_DarkArmorKnightBossCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_MatadorMageBossCharacter.cpp`
- `Project_Eden/Source/Project_Eden/Private/Game/GP_EnemySpawnVolume.cpp`
- `Project_Eden/Source/Project_Eden/Private/Game/GP_GameMode_ZoneCombat.cpp`
- `Project_Eden/Source/Project_Eden/Private/Utils/GP_BlueprintLibrary.cpp`
- `Project_Eden/Source/Project_Eden/Public/Characters/GP_DarkArmorKnightBossCharacter.h`

## Verification

- `Project_EdenEditor Win64 Development` and `Project_EdenServer Win64 Development` succeeded.
- `ProjectEden.Combat` passed 20/20, including friendly-fire and direct Dark Wave spawn; live two-client presentation remains.
- `ProjectEden.Game.EnemySpawnPlacement` passed 2/2 after the boss-center spawn change.

## Follow-up

_None._

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/douyun0623.md)
