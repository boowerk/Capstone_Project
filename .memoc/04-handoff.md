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
# Agent Handoff

Last synced: 2026-06-17T07:55:00+09:00

## Start Here

- Search memory before broad file reads: `memoc search "<topic>" --limit 5`.
- Use project-local wrapper if PATH/global runtime stalls: `.\.memoc\bin\memoc.cmd <command>`.
- Keep `session-summary.md` replace-only and under 800B. Put durable work in worklogs or this handoff file.

## Immediate Resume Items

- Basic enemies: C++ build succeeded and BP templates were created, but PIE behavior still needs validation. Check common BT chase/attack transitions, ranged hit distance, and flying movement/pathing.
- Boss routing: C++ guard routes boss pawns through `BossAttackExecution` even if the shared BT still calls `BTT_ExecuteEnemyAttack`. In editor, replace stale boss Attack nodes with `BTT_ExecuteBossAttack` when convenient.
- Crystal Seraph: native prototype builds. Make a BP child from `AGP_CrystalSeraphBossCharacter`, wire BT/BB/arena placement, then create BP visual children for prism, laser, wing core, shard projectile, and sanctuary marker actors.
- Skill augments: PIE-test cooldown, projectile count, element-gated DAs, damage multiplier, radius/range scaling, and active/impact VFX override priority.
- Lightning work: if resuming that feature, verify Lightning Storm periodic area settings, Niagara parameter forwarding, preview diameter, initial hit, periodic ticks, server/client agreement, and selected-position overlap.
- Motion matching: connect/validate `CHT_MM_MaskMan_Root`; verify idle, turn-in-place, run, sprint, in-air, and directional movement speeds.

## Known Setup Details

- Crystal Seraph optional BB keys: `WingCoreBreakCount`, `bCanExposeWingCore`, `bWingCoreExposed`, `CrystalPrismActor`, `bCanUseLaserPattern`, and `bCanUsePrismPattern`. Shared keys include `bIsGroggy`, `PreferredHoverHeight`, `PreferredAirRange`, and `bShouldTeleport`.
- Crystal Seraph damage states in `GP_DamageExecCalculation`: `CrystalGuarded` = 15%, `WingCoreExposed` = 50%, `Groggy` = full damage. Use `gp.DamageExec.Log 1` for verification.
- Augment fields: empty `TargetSkillTags` means global/all-skill; `RequiredElementTag` gates application by current tech element; `GrantedElementTag` changes current tech element and is separate.
- Augment visual priority: `AGP_PlayerState` resolves selected augment `ActiveVFXOverride` and `ImpactVisualActorOverride`; `UGP_SkillBase` applies the latest matching override before SkillData element/default visuals.
- Lightning Niagara names include spaces: `User.Spawn Count`, `User.SpawnRate`, `User.Large Radius`, and `User.Loop Duration`.
- Tech widget button names must remain exact for auto-bind: `Button_Pyros`, `Button_Hydro`, `Button_Volt`, `Button_Aero`, `Button_Lux`, `Button_Chaos`, `Button_Brute`.
- Boss HUD expected setup: `/Game/UI/HUD/WBP_PlayerHUDWidget.BossBar` should be a `WBP_BossBar` child from `UGP_AttributeWidget` configured to Health/MaxHealth.

## Blockers And Cautions

- If Live Coding/UBA reports `Low on memory`, prefer asking for an editor rebuild/restart instead of forcing more UBT builds.
- Avoid blind AnimGraph node creation through scripts for `ABP_UEFNSource_Player`; a previous `AnimGraphNode_ChooserPlayer` injection crashed the editor.
- Do not patch A-pose/default-pose bugs by changing RM/inertia end paths first. `ActionEnd` means control/input unlock, not montage end.
- Existing `Content/Maps/DemoMap/TestMap.umap` may be unloadable by commandlet.

## Verification Snapshot

- `Project_EdenEditor Win64 Development` build succeeded for the latest basic enemy/boss routing work.
- Basic enemy BP creation script ran, but commandlet returned failure because the existing test map was unloadable.
- Generic boss Attack task routing build succeeded; PIE still needs checking for expected boss-routing log.
