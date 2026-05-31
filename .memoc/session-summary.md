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
Last: 2026-06-01T00:29:00+09:00
Replace, do not append. Keep <800B.
History: worklog. Resume risks: 04-handoff.md.

## Status
- ActionEnd remains input/control unlock, not montage end. Source/target AnimBP slot paths restored after unsafe lower-body blend caused A-pose.
- Held move cancel path added: player caches move input while Fixed and Dash/DashSlash broadcast cancel at ActionEnd if held input is recent.
- Log read showed held cancel fired, but inertia skipped because carry decayed to ~54 from sprint entry ~854. Patched held-input handoff to seed CMC velocity from max(carry, entry speed, MaxWalkSpeed), with `[ActionRM][ApplyHeldInput]` log.
- For direction-change slip after roll, held-input handoff now also overwrites post-action anim velocity, and UEFNSource AnimInstance overrides future `GeneratedTrajectory` samples from that action velocity. Added `[ActionRM][AnimTrajectory]` log.
- User reported all motion feels good, but trajectory debug during roll shows odd vertical path. Narrowed trajectory override to post-action velocity hold only (`IsUsingPostActionAnimVelocity`) so roll itself keeps normal `CharacterTrajectoryComponent` path; override no longer runs during montage/RM playback.
- User saw rare stop stutter after sprint-roll then releasing input. Patched post-action anim velocity hold to only happen when movement input is still recent or action was cancelled by movement input; released-input natural completion clears the hold.

## Changed
_Recent durable changes only._

## Open Tasks
_Current open tasks only._

## Resume
- LiveCoding succeeded after released-input hold patch. Git status/diff over LFS assets can fail with `.git/lfs/tmp` access denied.
