---
memoc: true
type: worklog
scope: project-memory
created: 2026-07-24T01:57:40+09:00
updated: 2026-07-24T01:57:40+09:00
status: active
tags:
  - memoc
  - memoc/worklog
---
# remove fixed demo flow

actor: boowerk
actor_source: git config user.name
branch: main
status: done
created: 2026-07-24T01:57:40+09:00

## Summary

- Removed the seeded outer-to-center route and fixed Red Rift, Defense, Shrine, and Dark Armor Knight sequence.
- Removed the guided minimap objective, scripted event actors/DataAssets, and event-specific augment RPC.
- Preserved the three-player session/start contract, RegionState/PCG, ordinary zone progression code, and combat systems.
- Kept empty serialization-only RegionEvent types because protected map/data assets still reference them.

## Changed Files

- See commits `c78e88a9`, `ba0f2bfa`, and `29baec37`.
- `.memoc/02-current-project-state.md`
- `.memoc/03-decisions.md`
- `.memoc/04-handoff.md`
- `.memoc/session-summary.md`
- `docs/AgentTeamWorkflow.md`

## Verification

- `Project_EdenEditor Win64 Development` build passed.
- Landscape integrity, three-player runtime starts, minimap capture, and enemy production animation passed 4/4 under `NullRHI`.
- Live server and multiplayer PIE were intentionally not run.

## Follow-up

- `L_LandscapeMap` currently provides three-player free exploration because it has no authored enemy zones.
- Any replacement core loop or map destination requires a separate reviewed ticket.

## Related

- [Activity](../../../activity.md)
- [Worklog](../../README.md)
- [Actor](../../../actors/boowerk.md)
