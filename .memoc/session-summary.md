---
memoc: true
type: state
scope: project-memory
created: 2026-06-06T06:43:32
updated: 2026-06-06T22:35:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-06-06T23:05:00+09:00

## Status
- Matador BP now parented to `GP_MatadorMageBossCharacter`; BP/C++ defaults now point at dedicated `BT_Boss_Matador` + `BB_Boss_Matador`.
- DamageExec now checks target ASC tags directly, not only captured tags, for Matador guarded/groggy multiplier.
- Commit audit found no team-made Matador-specific BT asset; dedicated Matador BT/BB were created by duplicating boss common assets as a temporary base.
- Asset audit found `BT_Boss_Matador`/`BT_BossCommon` still contain generic `BTT_ExecuteEnemyAttack`, not boss `BTS_UpdateBossTactics`/`BTT_ExecuteBossAttack`; `BT_Boss_Sans` is the real boss BT template.

## Changed
- Fixed shared AI fallback paths to actual `/BT/Common/*`.
- Matador native constructor assigns dedicated Matador BT/BB defaults; BP asset also saved with these overrides.
- Temporary cleanup: bull impact disables collision after first hit, decoy no longer uses Pawn object type, Matador tactics suppress generic melee/summon and prefer range reposition/area/bull.
- Added temporary Matador fallback pattern loop: every 6s, if target is 450-2400cm away and no bull is active, spawn bull; target falls back to player pawn if BT/BB target is missing.
- Fallback loop default is now off; current broken Matador BT is handled by common `BTS_UpdateEnemyTactics` opening bull-pattern attack windows and `BTT_ExecuteEnemyAttack` selecting `Utility_MatadorBullPattern` for Matador.

## Open Tasks
- Live Coding compile. MCP UBT returned code 6 without useful project-log details.
- Later MCP UBT/Python calls were blocked by usage limit; compile must be done in editor/IDE.
- PIE check `gp.DamageExec.Log 1`: guarded should show `TargetGuarded=1 BossStateMult=0.10`; groggy should show `TargetGroggy=1 BossStateMult=1.00`.
- DataAsset plan: move Matador classes/tuning/damage/BT/BB into a dedicated boss config asset instead of scattered BP defaults.
- Replace temporary `BTS_UpdateBossTactics` hacks with real Matador BT/config once design is stable.

## Resume
- If damage number should appear at decoy impact, add hit-proxy location support; current ASC damage event reports boss avatar.
