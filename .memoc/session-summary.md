---
memoc: true
type: state
scope: project-memory
created: 2026-06-17T04:47:15
updated: 2026-06-17T16:24:00+09:00
status: active
tags: [memoc, memoc/state]
---
# Session Summary
Last: 2026-06-17T16:24:00+09:00
Replace, do not append. Keep <800B.

## Status
- Matador design: body floats/does nothing; decoy owns melee, bull lure/redirect, and bull counter loop.
- `BP_Boss_Matador` parent is `GP_MatadorMageBossCharacter`; `BP_MatadorDecoy` parent is `GP_MatadorBossDecoyActor`.

## Changed
- Created/assigned `BP_MatadorDecoy` as `BP_Boss_Matador.DecoyActorClass`.
- Bull spawns toward active decoy, no longer repositions decoy at bull start.
- Body stops AI movement while non-groggy; active decoy follows player when bull inactive.
- Decoy layout now matches Character baseline: capsule 34/100, mesh Z -100, actor Z uses NavMesh + 100.
- Decoy movement replication enabled; layout reapplied in Construction/BeginPlay to beat stale BP native defaults.

## Resume
- Compile in editor. Then verify placed boss instance does not override `DecoyActorClass` back to native class.
