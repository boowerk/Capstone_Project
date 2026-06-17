---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-06-17T07:55:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Current Project State

Last synced: 2026-06-17T07:55:00+09:00

## Active Systems

- Basic enemy inheritance path exists: `AGP_MeleeEnemyCharacter`, `AGP_RangedEnemyCharacter`, and `AGP_FlyingEnemyCharacter` are Blueprintable C++ parents with shared `BT_EnemyCommon`/`BB_EnemyCommon`, archetype movement/perception defaults, MaskMan prototype mesh, and default GAS attack tags.
- Basic enemy BP templates exist under `/Game/Characters/EnemyCharacter/Basic`: `BP_BasicEnemy_Melee`, `BP_BasicEnemy_Ranged`, and `BP_BasicEnemy_Flying`.
- Enemy attack execution is archetype-aware: `AGP_EnemyCharacter` grants a default attack ability, `UGP_EnemyRangedAttack` provides the ranged prototype, and shared `BTT_ExecuteEnemyAttack` resolves the controlled enemy default attack tag.
- Boss attack routing is centralized through `BossAttackExecution`. `BTT_ExecuteBossAttack` and boss pawns accidentally routed through generic `BTT_ExecuteEnemyAttack` use the same GAS pattern selector and failure logging.
- Crystal Seraph native prototype is implemented: `AGP_CrystalSeraphBossCharacter`, `UGP_CrystalSeraphStateComponent`, native pattern abilities, pattern actors, optional Blackboard keys, and damage-state multipliers are in place.
- Skill augment system supports duplicate prevention, element requirements, cooldown/radius/range/damage/projectile modifiers, and active/impact VFX overrides. Latest matching selected augment wins for visual overrides.
- Targeted skill foundation exists in C++ through `UGP_TargetedSkillBase`, with instant/projectile/ray/target-actor selection modes, preview/debug targeting, confirm/cancel, and server-forwarded selection events.
- XP/level basics exist on `AGP_PlayerState`; enemy death grants editable `XPReward`; augment UI flow is owned by `AGP_PlayerController`.
- Minimap capture now auto-spawns/follows when no placed capture actor exists, initializes its render target, and refreshes HUD binding through subsystem/controller/widget paths.
- Motion matching and root-motion work is mostly C++/asset integration state: UEFN source mesh drives animation, MaskMan retargets, chooser context variables exist, directional movement speed lives in the animation set profile, source fallback montages can drive capsule movement, and post-action velocity handoff logic exists.
- Run-progression system added (C++): spawn-volume-driven linear city progression. `AGP_EnemySpawnVolume` (placed-in-level box; holds ZoneOrder/DisplayName/bIsBossZone/Spawns and projects spawn points onto navmesh) defines each zone self-contained. `AGP_GameMode` (server) gathers all volumes, sorts by ZoneOrder, auto-spawns each zone's composition (reuses `GP_BossSummonAdds` SpawnActor+ProjectPointToNavigation+SpawnDefaultController pattern), tracks deaths via `AGP_EnemyCharacter::OnEnemyDied`, advances on clear. `AGP_GameState` replicates phase/zone/enemies-remaining for co-op HUD. BP hooks: OnZoneStarted (post-spawn setup), OnZoneCompleted (vegetation change + boss teleport), OnRunFinished (win/lose UI). Auto-starts after `StartDelaySeconds` (navmesh readiness) or via `StartRun()`. Not yet built/PIE-tested; `BP_ProjectEden_Gamemode` still needs reparenting to `AGP_GameMode`.
- memoc uses project-local `.memoc/runtime` first to avoid sandbox timeouts from global AppData runtime calls.

## Current Risks

- PIE/editor validation is still needed for basic enemy behavior, ranged reach, flying movement/pathing, Crystal Seraph setup, augment VFX/cooldown/projectile modifiers, and motion matching chooser playback.
- `BT_BossCommon.uasset` was observed still referencing `BTT_ExecuteEnemyAttack` in its Attack branch. C++ guards this, but the clean editor setup is to replace that node with `BTT_ExecuteBossAttack`.
- Existing `Content/Maps/DemoMap/TestMap.umap` has been reported unloadable by commandlet (`Invalid value for PACKAGE_FILE_TAG`).
- Direct tooling injection of new AnimGraph chooser nodes crashed before; prefer editor-authored graph changes for production AnimBP work.

## Open Tasks

- Vertical slice (priority): build the module, reparent `BP_ProjectEden_Gamemode` to `AGP_GameMode`, place `AGP_EnemySpawnVolume` actors (one per city + boss room, set ZoneOrder/Spawns; boss volume bIsBossZone), cover combat areas with `NavMeshBoundsVolume`, then PIE-test the city->boss->city loop + GameState HUD binding. Optionally wire `OnZoneCompleted` (vegetation + boss teleport)/`OnRunFinished`.
- Duplicate/subclass the three Basic enemy BP templates into concrete enemies; override perception/config, movement speed, and `Enemy|Abilities`.
- PIE-check common enemy BT chase/attack transitions, ranged hit distance, flying movement height/pathing, and boss generic-task routing logs.
- Create/verify Crystal Seraph BP child, BT/BB wiring, arena placement, and final meshes/materials/Niagara for prototype actors.
- PIE-test skill augment modifiers: cooldown, projectile count, element gate, damage multiplier, radius/range scaling, and visual overrides.
- Continue Lightning Storm/LightningStrike setup and periodic-area validation if working that branch.
- Point the active anim path at `CHT_MM_MaskMan_Root`, validate idle/TIP/run/sprint/in-air, and retire temporary enum/MM fallback only after the chooser is stable.

## References

- Detailed history belongs in `.memoc/worklog/` and `.memoc/session-summary-archive.md`.
- Durable rules/preferences live in `.memoc/03-decisions.md` and `.memoc/06-project-rules.md`.
