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
Last: 2026-06-07T00:00:00+09:00

## Status
- Created `/Game/Fab/M_WorldAlignedProjection` world-aligned master material and generated all 17 Fab Megascans world-aligned instances under `/Game/Fab/WorldAlignedMaterials`.
- Matador BP now parented to `GP_MatadorMageBossCharacter`; BP/C++ defaults now point at dedicated `BT_Boss_Matador` + `BB_Boss_Matador`.
- DamageExec now checks target ASC tags directly, not only captured tags, for Matador guarded/groggy multiplier.
- Commit audit found no team-made Matador-specific BT asset; dedicated Matador BT/BB were created by duplicating boss common assets as a temporary base.
- Asset audit found `BT_Boss_Matador`/`BT_BossCommon` still contain generic `BTT_ExecuteEnemyAttack`, not boss `BTS_UpdateBossTactics`/`BTT_ExecuteBossAttack`; `BT_Boss_Sans` is the real boss BT template.
- Added explicit C++ BT nodes for manual editor wiring: `BTS_UpdateMatadorTactics` and `BTT_ExecuteMatadorPattern`.

## Changed
- Added `/Game/Fab/WorldAlignedMaterials/MI_WA_*` instances for every complete Megascans Base/Normal/ORM texture set and assigned the Arcade arch static mesh slots to instances from that folder.
- Adjusted `M_WorldAlignedProjection` color response: removed direct AO output darkening and added `BaseBrightness`/`BaseTint`; WorldAlignedMaterials instances default to `BaseBrightness=1.25`.
- Restored original `MI_xdhhdgq` only for comparison, tuned WA defaults to `BaseBrightness=1.6`, `RoughnessMultiplier=0.75`, `RoughnessAdd=0.08`, and left two arcade compare actors in the current level: original-left and WA-right.
- The restored original MI had no parent and rendered checkerboard, so comparison now uses `MI_UV_ujdiddfew_Compare` (UV-based Stone Wall) on the left arcade actor and `MI_WA_ujdiddfew` on the right arcade actor.
- Fixed top-face stretching in `M_WorldAlignedProjection` by using `WorldAlignedTexture` `XYZ Texture` output for BaseColor instead of `XY Texture`.
- Replaced the unstable WA master with clean `/Game/Fab/M_WorldAlignedProjection_Clean` using constant V3 texture size; reparented all 17 `MI_WA_*` instances to it.
- Reduced visible projection stretching on angled/arched faces by lowering `ProjectionBlendContrast` to `0.35` and tightening WA tile size to `140`.
- User edited `/Game/Fab/M_WorldAlignedProjection`; all 17 `/Game/Fab/WorldAlignedMaterials/MI_WA_*` instances were reparented back to that master with Base/Normal/ORM textures preserved.
- Fixed shared AI fallback paths to actual `/BT/Common/*`.
- Matador native constructor assigns dedicated Matador BT/BB defaults; BP asset also saved with these overrides.
- Temporary cleanup: bull impact disables collision after first hit, decoy no longer uses Pawn object type, Matador tactics suppress generic melee/summon and prefer range reposition/area/bull.
- Added temporary Matador fallback pattern loop: every 6s, if target is 450-2400cm away and no bull is active, spawn bull; target falls back to player pawn if BT/BB target is missing.
- Fallback loop default is now off; Matador pattern should now be driven by explicit `BTS_UpdateMatadorTactics` + `BTT_ExecuteMatadorPattern` once user wires them into `BT_Boss_Matador`.
- Removed hidden generic attack override path in favor of the explicit Matador BT task/service.

## Open Tasks
- Live Coding compile. MCP UBT returned code 6 without useful project-log details.
- Later MCP UBT/Python calls were blocked by usage limit; compile must be done in editor/IDE.
- Direct UBT compile passed C++ compile for new node after fix; final link failed only because open UnrealEditor locked `UnrealEditor-Project_Eden.dll`.
- PIE check `gp.DamageExec.Log 1`: guarded should show `TargetGuarded=1 BossStateMult=0.10`; groggy should show `TargetGroggy=1 BossStateMult=1.00`.
- DataAsset plan: move Matador classes/tuning/damage/BT/BB into a dedicated boss config asset instead of scattered BP defaults.
- Replace temporary `BTS_UpdateBossTactics` hacks with real Matador BT/config once design is stable.

## Resume
- If damage number should appear at decoy impact, add hit-proxy location support; current ASC damage event reports boss avatar.
