---
memoc: true
type: state
scope: project-memory
updated: 2026-07-24T06:02:46+09:00
status: active
tags:
  - memoc
  - memoc/state
created: 2026-07-18T07:01:20
---
# Session Summary

- `9796240f`/`c971a3c2` tune regular-enemy death grains to fall before slowly curving into the latched player's chest.
- `SpriteSize=10` is preserved. Gravity is full through `.28s`, fades by `.60s`; attraction starts `.38s`, ramps `.80s to strength 800`, with drag `1.4`, playback `2.6x`, and stop `1.90s`.
- Editor build plus absorption policy/asset and death-lifecycle tests pass; Niagara warning/error/NaN scans are clean.
- Manual PIE visual tuning remains; no server/PIE run by request.
- The previously removed fixed demo/event flow remains removed.
