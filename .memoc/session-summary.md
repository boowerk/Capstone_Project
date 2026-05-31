---
memoc: true
type: state
scope: project-memory
created: 2026-05-31T12:33:05
updated: 2026-05-31T23:14:00+09:00
status: active
tags:
  - memoc
  - memoc/state
---
# Session Summary
Last: 2026-05-31T23:31:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- ActionEnd remains input/control unlock, not montage end. Source/target AnimBP slot paths restored after unsafe lower-body blend caused A-pose.
- Held move cancel path added: player caches move input while Fixed and Dash/DashSlash broadcast cancel at ActionEnd if held input is recent.
- Log read showed held cancel fired, but inertia skipped because carry decayed to ~54 from sprint entry ~854. Patched held-input handoff to seed CMC velocity from max(carry, entry speed, MaxWalkSpeed), with `[ActionRM][ApplyHeldInput]` log.
- For direction-change slip after roll, held-input handoff now also overwrites post-action anim velocity, and UEFNSource AnimInstance overrides future `GeneratedTrajectory` samples from that action velocity. Added `[ActionRM][AnimTrajectory]` log.

## Changed
_Recent durable changes only._

## Open Tasks
_Current open tasks only._

## Resume
- LiveCoding succeeded after trajectory patch. Test sprint-held roll with direction change; expect ApplyHeldInput seed and AnimTrajectory logs.
