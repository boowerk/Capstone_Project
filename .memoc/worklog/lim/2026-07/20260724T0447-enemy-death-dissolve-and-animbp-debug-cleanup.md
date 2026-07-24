---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T04:47:31
updated: 2026-07-24T04:47:31
status: active
tags:
  - memoc
  - memoc/worklog
---
# Enemy death dissolve and AnimBP debug cleanup

actor: lim
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T04:47:31

## Summary

- Added death-only, per-enemy slot dissolve material switching for regular enemies and bosses while preserving existing death particles.
- Added per-enemy absorption materials and boss fragment dissolve handling; authored skeletal props keep their original materials.
- Disabled persisted UEFN/MaskMan AnimBP debug output.

## Changed Files

- `Project_Eden/Source/Project_Eden/{Public,Private}/VFX/GP_EnemyDeathAbsorptionComponent.*`
- `Project_Eden/Source/Project_Eden/{Public,Private}/VFX/GP_DeathVFXSetupLibrary.*`
- `Project_Eden/Source/Project_Eden/{Public,Private}/Actors/GP_BossDeathPresentationActor.*`
- `Project_Eden/Source/Project_Eden/Private/Characters/GP_EnemyCharacter.cpp`
- `Project_Eden/Scripts/Editor/{setup_enemy_death_materials,disable_player_anim_debug}.py`
- Production enemy/boss Blueprints, player AnimBPs, Niagara system, and generated `EnemyMaterials`

## Verification

- Rider-equivalent `Project_EdenEditor Win64 Development` build succeeded.
- `ProjectEden.VFX.EnemyDeathAbsorption` 4/4, boss death presentation 3/3, and enemy lifecycle 1/1 passed.
- Material creation, BP assignment, and idempotent AnimBP debug commandlets exited 0.

## Follow-up

- Three-player PIE visual/timing check remains.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/lim.md)
